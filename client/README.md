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

config.json
-----------

catalogPrefix - This is where you are doing your catalog queries. This should be the root of where we are querying in ndn.

faceConfig - Configure where you have your backend running and on which port it is listening. To setup a new backend, please read the readme at the root of the repo.


Changing the theme
------------------

Currently the theme is a modified bootstrap theme that is running larger fonts and custom colors.

If you would like to modify the theme go to [this url](http://bootstrap-live-customizer.com/). To modify the current theme, then upload the variables.less in this folder, make your modifications, and overwrite the variables.less file when you are done (and the theme.min.css).


