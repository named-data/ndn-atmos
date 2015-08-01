NDN Catalog (NDN Query and Retrieval Tool)
==========================================

This is the front end to the catalog which contains all of the client html and code.

Setup
-----

To simply run the client code, you will need the following things setup:

* A NDN backend running somewhere (The default config is pointed at a test backend)
* NDN-JS
  + Run `git submodule init ndn-js` in the client directory.
  + Then run `git submodule update`
* Configure the config.json
  + If it doesn't exist, you will need to copy it from the config-example.json
  + config.json is intentionally left out of the git to prevent overwriting it.

