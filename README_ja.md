# PowerDNS RecursorのType Bit Mapsの実装に起因するメモリ使用量の問題

## 概要

* PowerDNS Recursor 4.2.0未満には、NSECもしくはNSEC3のType Bit Mapの実装に問題があり、
  特殊なリソースレコードをキャッシュすると想定以上のメモリ（リソースレコードあたり3MB)を消費します。
* PowerDNS Recursorには、キャッシュのエントリ数を制限する機能はありますが、
  メモリ使用量でキャッシュを制限する機能はありません
* PowerDNS Recursorに特殊なリソースレコードを多くキャッシュさせることで、
  管理者の想定以上にメモリを消費し、サービスのパフォーマンスの低下や停止が発生します。

## 影響

攻撃者は、特殊なNSEC/NSECレコードを応答する攻撃用のドメイン名と権威サーバを用意し、
攻撃対象のPowerDNS Recursorへ攻撃用のドメイン名の問い合わせを送信し続けることで、
攻撃対象のサーバのメモリ使用量を増加させることができます。

## 対象

* PowerDNS Recursor 4.2.0未満(DNSSEC Validationの有無は関係ありません)

## 回避策

リソースレコードあたり3MBのメモリを消費する前提で、
キャッシュのエントリ数を制限します([max-cache-entries](https://doc.powerdns.com/recursor/settings.html#setting-max-cache-entries) )。

## 対策

PowerDNS Recursor 4.2.0へバージョンアップします。

## 詳細

### NSEC

DNSSECおいてドメイン名もしくはRRSetが存在しないことを証明するために、NSEC/NSECリソースレコードが導入されました。
NSECレコードでは、

* リソースレコードのOwerとNext Domainフィールドを用いて、その間にドメイン名が存在しないこと示します。
* Type Bit Maps Fieldにて、Onwerに存在するリソースレコードタイプを示します。

### Wire Format of Type Bit Maps

Type Bit MapｓのWire Formatは、リソースレコードタイプ(16bit)の配列ではなく、サイズがより小さくなるように定義されています
([4.1.2.  The Type Bit Maps Field](https://tools.ietf.org/html/rfc4034#section-4.1.2) )。

### PowerDNS RecursorのType Bit Mapsの実装

4.2.0未満のPowerDNS Recursorでは、Type Bit Mapsの値をC++のSTLのstd::set<uint16_t>で保持しています
([class NSECRecordContent](https://github.com/PowerDNS/pdns/blob/rec-4.1.14/pdns/dnsrecords.hh#L506) )。
Type Bit MapsのTypeの1bitが、std::set<uinit16_t>の1エントリに対応しています。


```c++
class NSECRecordContent : public DNSRecordContent
{
public:
  static void report(void);

// snip

DNSName d_next;
  std::set<uint16_t> d_set;
private:
};
```

C++(CentOS 7.6のGCC 4.8.5)のSTLはstd::setはRed-Black treeを用いて実装しているため、
std::setの一つのエントリには、Colorと親ノード、子ノードx2へのポインタが付属します。

```c++
  struct _Rb_tree_node_base
  {
    typedef _Rb_tree_node_base* _Base_ptr;
    typedef const _Rb_tree_node_base* _Const_Base_ptr;

    _Rb_tree_color      _M_color;
    _Base_ptr           _M_parent;
    _Base_ptr           _M_left;
    _Base_ptr           _M_right;
```

Type Bit Mapsの全てのbitを1とするとWireFormatでは8704bytesになりますが、PowerDNS Recursor上ではおよそ3MB程度になります。
そのため通常の署名済みゾーンのNSECレコードでは問題になりませんが、故意に多くのbitを1にした場合にPowerDNS Recursor のメモリ
使用量は非常に大きくなります。

```text
Wire Format
size of Type Bit Map = bit map count x ( Window Block + Bitmap Length + Bitmap ) bytes
                     = 256 x ( 1 + 1 + 32 ) bytes
                     = 8704 bytes

On memory
size of Type Bit Map = ( node size of red-black tree ) * 65536 + Overhead bytes
                     = 40 x 65536 + Overhead bytes
                     = 2,621,440 + Overhead bytes
                     ~ 3MB
```

sample code to estimate memory usage: [set-uint16_t-x100.cpp](https://raw.githubusercontent.com/sischkg/huge_nsec_response/master/set-uint16_t-x100.cpp)

### PowerDNS Recursorでのキャッシュの制限

BINDやUnboundでは、リソースレコードのキャッシュの量をメモリ使用量で制限することが出来ますが、
PowerDNS Recursorではキャッシュ内のリソースレコードの数で制限します。PowerDNS Recursorで、
NSECのメモリ使用量(リソースレコードあたり3MB)に従ってエントリ数を制限すると、キャッシュを
多く持つことが出来なくなります。


### 対策

以下のPull Requestがマージされ、PowerDNS 4.2.0にて修正されています。

https://github.com/PowerDNS/pdns/pull/7345


# おまけ

## Type Bit Mapsのテキスト表現について

PowerDNS Recursorだけではなく他のリゾルバにも関係します。

上記の通りType Bit MapsのWire Formatはサイズが小さくなるように定義されています。
Type Bit Mapsのすべてのbitを1にしたNSECレコードをテキスト形式に変換すると
620KB以上のサイズになります[応答例](https://raw.githubusercontent.com/sischkg/huge_nsec_response/master/nsec_response.txt)。
このようなNSECレコードを多くキャッシュしたフルリゾルバでキャッシュをダンプ(`rndc dumpdb`)すると、
フルリゾルバのメモリ使用量と比較し非常に大きなファイルが作成されます。

## 例: 1000レコードをキャッシュした場合

* フルリゾルバ: BIND
* メモリ使用量: 30MB
* ダンプファイルのサイズ; 644MB

