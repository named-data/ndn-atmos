NDN Catalog (NDN Query and Retrieval Tool)
==========================================

This is the front end to the catalog which contains all of the client html and code.

Setup
-----

Simply run a webserver on THIS directory (aka
.../ndn-atmos/client) and point clients to /catalog on either the domain or
IP that the server is running on.

Good suggestions are http-server in npm, pythons SimpleHTTPServer, or a standard
webserver like nginx or apache.

__Note:__
HTTPS is not supported unless the ndn websocket server is running a valid
certificate as well as the web server. This is due to a security rule in most
browsers that restricts the ws protocol from running in https tabs/frames. All
content in the https frame MUST be secure including the websocket.
(HTTPS is untested at this time but you can ask questions on the mailing list
if you have problems.)

config.json
-----------

###Global
* CatalogPrefix - Where should the catalog attach in the URI scheme? (Usually the
root of a catalog)
* FaceConfig - A valid NDN node location running the websocket for NDN-JS.

###Retrieval
* DemoKey - The public and private portion of an RSA in Base64. This key must be
valid in the NDN Network for it to work.
* Destinations - A list of retrieval URIs. These must be running the retrieval
code or retrieval will fail.

Changing the theme
------------------

Currently the theme is a modified bootstrap theme that is running larger fonts
and custom colors.

If you would like to modify the theme go to [this url](http://bootstrap-live-customizer.com/).
To modify the current theme, then upload the variables.less in this folder, make
your modifications, and overwrite the variables.less file when you are done
(and the theme.min.css).


