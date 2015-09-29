## Boost setup

1. Download boost from [boost.org](http://www.boost.org/).
1. Unpack boost to some place.
1. Run either .\bootstrap.bat (on Windows), or ./bootstrap.sh (on other operating systems) under boost folder.

## Boost build (Build the necessary subset only)

#### Windows (or other mainstream desktop platforms shall work too):
Run with following script will build the necessary subset:

```bash
bjam install --prefix="<your boost install folder>" --with-system --with-date_time --with-random link=static runtime-link=shared threading=multi
```
Optionally You can merge all output .lib files into a fat one,especially if you're not using cmake.

In output folder, run:

```bash
lib.exe /OUT:boost.lib *
```
