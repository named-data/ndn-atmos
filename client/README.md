NDN Catalog (NDN Query and Retrieval Tool)
==========================================

This is the front end to the catalog which contains all of the client html and code.

Setup
-----

To setup the site including the production folder simply run `./autogen.sh`

The site will now be available in the catalog directory.

If you run into issues of requiring sudo access you may need to [configure the npm prefix](http://competa.com/blog/2014/12/how-to-run-npm-without-sudo/) to point your global package repo somewhere else. Should you find yourself in a situation that you don't mind using sudo, feel free to simply just run with sudo.

##Serving the site:

To serve the site, point a webserver at the same directory this README file is in. Then give users the url: http://<your domain>/catalog or /catalog-dev depending on if you are running deployment code. 

HTTPS is not supported and will break the code as it is unless the ndn backend is running a valid certificate as well, this is due to a security rule in most browsers that restricts the ws protocol from running in https tabs/frames. All content in the https frame MUST be secure. (Aka run wss in https) (HTTPS is not officially supported)

config.json
-----------

###Global
* CatalogPrefix - Where should the catalog attach in the URI scheme? (Usually the root of a catalog)
* FaceConfig - A valid NDN node location running the websocket for NDN-JS.

###Retrieval
* DemoKey - The public and private portion of an RSA in Base64. This key must be valid in the NDN Network for it to work.
* Destinations - A list of retrieval URIs. These must be running the retrieval code or retrieval will fail.

Changing the theme
------------------

Currently the theme is a modified bootstrap theme that is running larger fonts and custom colors.

If you would like to modify the theme go to [this url](http://bootstrap-live-customizer.com/). To modify the current theme, then upload the variables.less in this folder, make your modifications, and overwrite the variables.less file when you are done (and the theme.min.css).


