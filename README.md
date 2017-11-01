# BTCAddr(esses)Gen(erator)

I'm going to make a bitcoin address generate program/library.

You can import the generated private key to bitcoin wallet.

Yes, nothing more.

# How to use?

## Requirements.

* You need install `secp256k1` library before you compile source.
* Clone and install it from `https://github.com/bitcoin-core/secp256k1.git`

## Compile

* Run in console `mkdir build && cd build && cmake .. && make -j3`

## Run

* Run in console `build/btcaddrgen`
* It will show the new address generated with private key (public key also displayed).

## Sample

```
address: 1KdmnoWbqHxqoER9ADLuw7FndRDsLJ33KZ
public key: mUFUkT1VsghGCwkTvPveybbv6PYVaDemEJuqWYjuHMKu
private key: BPnJQQztyo4RrPERL7WL15h1pm1GzzcvHb5ZjhqGUhz6
```