ndn-atmos
============

### This is pre-release software
####If you have trouble running it, subscribe to <a href=http://www.netsec.colostate.edu/mailman/listinfo/ndn-sci> ndn-sci mailing list </a> and send an email.

 This software is designed to support ongoing climate model research at Colorado State University,
 Berkeley and other institutes. Future plan includes porting this tool to suite other scientific
 community such as High Energy Particle Physics.

 Currently, this software provides an API to publish, query and retrieve scientific datasets using
 NDN.

Dependencies
---------------------

**The ndn-atmos is built based on several libraries**

 * boost (Minimum required boost version is 1.48.0)
 * jsoncpp 1.6.0 (https://github.com/open-source-parsers/jsoncpp.git)
 * mysql 5.6.23 (http://www.mysql.com/)
 * ndn-cxx (https://github.com/named-data/ndn-cxx.git)
 * ChronoSync (https://github.com/named-data/ChronoSync.git)

**Dependency for tools and translator library**

 * python3
 * netcdf4-python3
 * mysql-connector-python3

**The ndn-cxx and ChronoSync need some other prerequisites.**

 *  For OSX, the prerequisites can be installed using Homebrew:

<pre>
    brew install boost sqlite3 mysql jsoncpp hdf5 openssl cryptopp protobuf
    pip3 install mysql-connector-python --allow-all-external
    pip3 install netCDF4

</pre>

 * For Ubuntu, use the command below to install the prerequisites:

<pre>
    sudo apt-get install libboost-all-dev libssl-dev libcrypto++-dev \
                        libsqlite3-dev libmysqlclient-dev libjsoncpp-dev \
                        protobuf-compiler libprotobuf-dev netcdf4-python \
                        python3-mysql.connector python3-pip libhdf5-dev \
                        libnetcdf-dev python3-numpy

    sudo pip3 install netCDF4
</pre>

 * For Fedora, use the command below to install the prerequisites:

<pre>
    sudo yum install boost-devel openssl-devel cryptopp-devel sqlite3x-devel \
                    mysql-devel jsoncpp-devel protobuf-compiler protobuf-devel \
                    netcdf4-python3 mysql-connector-python3
</pre>



Installing ndn-cxx
---------------------

* Download ndn-cxx source code. Use the link below for ndn-cxx code:

<pre>
  git clone https://github.com/named-data/ndn-cxx.git
  cd ndn-cxx
  git checkout -b shared_library 7ed294302beee4979e97ff338dee0eb3eef51142
</pre>

* In library folder, build from the source code

<pre>
  ./waf configure --disable-static --enable-shared
  ./waf
  ./waf install
</pre>

Installing ChronoSync
---------------------

* Download ChronoSync source code. Use the link below for the ChronoSync code:

<pre>
  git clone https://github.com/named-data/ChronoSync.git
  cd ChronoSync
</pre>

* Build from the source code

<pre>
  ./waf configure
  ./waf
  ./waf install
</pre>


Installing ndn-atmos
---------------------

Follow the steps below to compile and install ndn-atmos:

* Download the ndn-atmos source code. Use the command below:

<pre>
  git clone https://github.com/named-data/ndn-atmos.git
  cd ndn-atmos
</pre>

* Build ndn-atmos in the project folder

<pre>
  ./waf configure
  ./waf
  ./waf install
</pre>

* To test ndn-atmos, please use the steps below:

<pre>
  ./waf configure --with-tests
  ./waf
  ./build/catalog/unit-tests
</pre>

* Note that if you are using Fedora or Ubuntu, you may need to create a configuration file for
ndn-cxx in /etc/ld.so.conf.d to include the path where the libndn-cxx.so is installed. Then
update using `ldconfig`

*For example, if the libndn-cxx.so is installed in /usr/local/lib64, you need to include
this path in a "ndn-cxx.conf" file in /etc/ld.so.conf.d directory, and then run "ldconfig".


Running ndn-atmos
--------------------------

Install translator library
---------------------------
1. For the translator, ndn_cmmap_translator library is required to be in PYTHONPATH

<pre>
     export PYTHONPATH="full path to /ndn-atmos/lib":$PYTHONPATH
</pre>


Initializing Database
---------------------
* Create a database using standard mysql tool.
* You also need to create a user and set a password to connect to the database the database.
* Note that you will need to have actual CMIP5 data to run the tool.
* Run

<pre>
    python3 insert_names.py
</pre>

* Input full path to the filename and config file to translate
* A CMIP5 config file is located under
</pre> /ndn-atmos/lib/ndn_cmmap_translators/etc/cmip5/cmip5.conf ```
* This will create a table named cmip5 and insert the names into the table


Starting NFD
------------
NFD is the NDN forwarding daemon.

* Download NFD source code. Use the link below for the NFD code:

<pre>
  git clone https://github.com/named-data/NFD.git
  cd NFD
  git checkout NFD-0.3.2
  git submodule init && git submodule update
</pre>

* Build NFD

<pre>
  ./waf configure
  ./waf
  ./waf install
</pre>

* Run NFD

<pre>
    nfd-start
</pre>

* Note that if you are using Fedora or Ubuntu, you may need to create a configuration file for
ndn-cxx in /etc/ld.so.conf.d to include the path where the libndn-cxx.so is installed. Then
update using command below:

<pre>
  ldconfig
</pre>

*For example, if the libndn-cxx.so is installed in /usr/local/lib64, you need to include
this path in a "ndn-cxx.conf" file in /etc/ld.so.conf.d directory, and then run "ldconfig".


Launching atmos-catalog
-----------------------

* Make sure database is initialized and running

* Create catalog configuration file

<pre>
    cp /usr/local/etc/ndn-atmos/catalog.conf.sample /usr/local/etc/ndn-atmos/catalog.conf
</pre>

* Edit the configuration file /usr/local/etc/ndn-atmos/catalog.conf. Modify the database parameters
in both the queryAdapter and publishAdapter sections.
* Note that the database parameters in these two sections may be different to provide different
privileges.


* Run ndn-atmos

<pre>
    atmos-catalog
</pre>


Starting front end
------------------

* Open the client folder in ndn-atmos

* Checkout the ndn-js in the client folder. Use the link blow:

<pre>
  git clone http://github.com/named-data/ndn-js.git
  cd ndn-js
  git checkout v0.8.1
</pre>

* Start python simple server in the client folder (ndn-atmos/client)

<pre>
  python -m SimpleHTTPServer
</pre>

* Open project query page in a web browser

<pre>
     http://localhost:8000/query/query.html
</pre>
