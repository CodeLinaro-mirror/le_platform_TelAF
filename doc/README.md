# Build offical API reference locally

## Setup the API compilation environment
```bash
cd ~/telaf
source set_af_env.sh sa525m
bin/legs
```

## Generate the source of all .api files
```bash
cd ~/telaf/doc
./makedoc
```

## Generate the Sphinx sources from Doxygen
```bash
doxygen telafDoxyConfig
```

## Create the local qdoc site
NOTE: make sure the python used is greater than 3.0, otherwise use virtual environment.
```bash
python3 -m venv myenv
source myenv/bin/activate
make setup
make html
```

The local site is at _build/html. Open index.html with any browser.