# MachiKania ports
MachiKania用のmicropythonです。
現状はtypeMのみサポートしていますが容量的にはtypeZでもシュリンク、mips16e化すればいけそうです。

## 使い方
Makefileの`CROSS_COMPILE = /~~~~~`部分を自前のc compilerに置き換えてください。
```sh
make
```
と叩いて、build/firmware.hexを書き込んでください。
ブートローダ対応させるためには`app_p32MX370F512H.ld`が必要です。これは、[MachiKaniaの公式ページ](http://www.ze.em-net.ne.jp/~kenken/machikania/typem.html)にあります。

`firmware.hex`は[リリースページ](https://github.com/elect-gombe/micropython/releases/download/v1.9.4/firmware.hex)に載せておきます。ブートローダを通して書き込む人用です。ビルドがだるければこれ使ってもいいですよ。
コンパイラは[win](https://github.com/elect-gombe/win_pic32_videogame_DE),[Linux32](https://github.com/elect-gombe/pic32_videogame_DE)
にあるやつで通ると思います。

xc32ではgcc99、(c99ですらも）をすべてサポートしていないため、ビルドが通りません。サポートを試みましたがもうだめです。特にmath.hがめちゃくちゃで、もうこんなクソコンパイラはだめだ。


## TODO
### filesystem
- [ ] filesystem対応
  - [ ] SD
  - [ ] Internal Flash Storage(160kB程度かな？)対応

### peripheral
- [ ] GPIO
- [ ] Push Switch
- [ ] SPI
- [ ] UART
- [ ] I2C

### Video composite
- [ ] Text
- [ ] Graphics

### others
- [ ] Interrupt abortion
- [ ] a.out execution(shと叩いてから起動するようにするかな？それとも特殊な記号列後にするかも?)
  - [ ] Text Editor support(特殊なaliasでREPLからでも起動できるようにする)
  - [ ] other file command
  - [ ] smlrc, build toolsのports(C言語のサポート, pythonから操作できるようにできればいいなぁ)


## やりたいことたくさんある。開発者ゆる募
