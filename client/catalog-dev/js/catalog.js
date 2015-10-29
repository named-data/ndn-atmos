//Run when the document loads AND we have the config loaded.
(function(){
  "use strict";
  var config;
  Promise.all([
    new Promise(function(resolve, reject){
      $.ajax('config.json').done(function(data){
        config = data;
        resolve();
      }).fail(function(){
        console.error("Failed to get config.");
        ga('send', 'event', 'error', 'config');
        reject();
      });
    }),
    new Promise(function(resolve, reject){
      var timeout = setTimeout(function(){
        console.error("Document never loaded? Something bad has happened!");
        reject();
      }, 10000);
      $(function () {
        clearTimeout(timeout);
        resolve();
      });
    })
  ]).then(function(){
    new Atmos(config);
  }, function(){
    console.error("Failed to initialize!");
    ga('send', 'event', 'error', 'init');
  });
})();

var Atmos = (function(){
  "use strict";

  var closeButton = '<button type="button" class="close" data-dismiss="alert" aria-label="Close"><span aria-hidden="true">&times;</span></button>';

  var guid = function(){
    var d = new Date().getTime();
    var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
      var r = (d + Math.random()*16)%16 | 0;
      d = Math.floor(d/16);
      return (c=='x' ? r : (r&0x3|0x8)).toString(16);
    });
    return uuid;
  }

  /**
   * Atmos
   * @version 2.0
   *
   * Configures an Atmos object. This manages the atmos interface.
   *
   * @constructor
   * @param {string} catalog - NDN path
   * @param {Object} config - Object of configuration options for a Face.
   */
  var Atmos = function(config){

    //Internal variables.
    this.results = [];
    this.resultCount = Infinity;
    this.name = null;
    this.page = 0;
    this.resultsPerPage = 25;
    this.retrievedSegments = 0;

    //Config/init
    this.config = config;

    this.catalog = config['global']['catalogPrefix'];

    this.face = new Face(config['global']['faceConfig']);

    //Easy access dom variables
    this.categories = $('#side-menu');
    this.resultTable = $('#resultTable');
    this.filters = $('#filters');
    this.searchInput = $('#search');
    this.searchBar = $('#searchBar');
    this.searchButton = $('#searchButton');
    this.resultMenu = $('.resultMenu');
    this.alerts = $('#alerts');
    this.requestForm = $('#requestForm');

    var scope = this;

    $('.requestSelectedButton').click(function(){
      ga('send', 'event', 'button', 'click', 'request');
      scope.request(scope.resultTable.find('.resultSelector:checked:not([disabled])').parent().parent());
    });

    this.filterSetup();

    //Init autocomplete
    this.searchInput.autoComplete(function(field, callback){
      ga('send', 'event', 'search', 'autocomplete');
      scope.autoComplete(field, function(data){
        var list = data.next;
        var last = data.lastComponent === true;
        callback(list.map(function(element){
          return field + element + (last?"/":""); //Don't add trailing slash for last component.
        }));
      });
    });

    //Handle search
    this.searchBar.submit(function(e){
      ga('send', 'event', 'search', 'submit');
      e.preventDefault();
      if (scope.searchInput.val().length === 0){
        if (!scope.searchBar.hasClass('has-error')){
          scope.searchBar.addClass('has-error').append('<span class="help-block">Search path is required!</span>');
        }
        return;
      } else {
        scope.searchBar.removeClass('has-error').find('.help-block').fadeOut(function(){$(this).remove()});
      }
      scope.pathSearch();
    });

    this.searchButton.click(function(){
      console.log("Search Button Pressed");
      ga('send', 'event', 'button', 'click', 'search');
      scope.search();
    });

    //Result navigation handlers
    this.resultMenu.find('.next').click(function(){
      ga('send', 'event', 'button', 'click', 'next');
      if (!$(this).hasClass('disabled')){
        scope.getResults(scope.page + 1);
      }
    });
    this.resultMenu.find('.previous').click(function(){
      ga('send', 'event', 'button', 'click', 'previous');
      if (!$(this).hasClass('disabled')){
        scope.getResults(scope.page - 1);
      }
    });
    this.resultMenu.find('.clearResults').click(function(){
      ga('send', 'event', 'button', 'click', 'resultClear');
      scope.clearResults();
      $('#results').fadeOut(function(){
        $(this).addClass('hidden');
      });
    });

    //Change the number of results per page handler
    var rpps = $('.resultsPerPageSelector').click(function(){

      var t = $(this);

      if (t.hasClass('active')){
        return;
      }

      rpps.find('.active').removeClass('active');
      t.addClass('active');
      scope.resultsPerPage = Number(t.text());
      scope.getResults(0); //Force return to page 1;

    });

    //Init tree search
    $('#treeSearch div').treeExplorer(function(path, callback){
      console.log("Tree Explorer request", path);
      ga('send', 'event', 'tree', 'request');
      scope.autoComplete(path, function(data){
        var list = data.next;
        var last = (data.lastComponent === true);
        console.log("Autocomplete response", list);
        callback(list.map(function(element){
          return (path == "/"?"/":"") + element + (!last?"/":"");
        }));
      })
    });

    $('#treeSearch').on('click', '.treeSearch', function(){
      var t = $(this);

      scope.clearResults();

      var path = t.parent().parent().attr('id');

      console.log("Stringing tree search:", path);

      scope.query(scope.catalog, {'??': path},
      function(interest, data){ //Success
        console.log("Tree search response", interest, data);

        scope.name = data.getContent().toString().replace(/[\n\0]+/g,'');

        scope.getResults(0);
      },
      function(interest){ //Failure
        console.warn("Request failed! Timeout", interest);
        scope.createAlert("Request timed out.\""+ interest.getName().toUri() + "\" See console for details.");
      });

    });

    this.setupRequestForm();

  }

  Atmos.prototype.clearResults = function(){
    this.results = []; //Drop any old results.
    this.retrievedSegments = 0;
    this.resultCount = Infinity;
    this.page = 0;
    this.resultTable.empty();
  }

  Atmos.prototype.pathSearch = function(){
    var value = this.searchInput.val();

    this.clearResults();

    var scope = this;

    this.query(this.catalog, {"??": value},
    function(interest, data){
      console.log("Query response:", interest, data);

      scope.name = data.getContent().toString().replace(/[\n\0]/g,"");

      scope.getResults(0);

    },
    function(interest){
      console.warn("Request failed! Timeout", interest);
      scope.createAlert("Request timed out. \"" + interest.getName().toUri() + "\" See console for details.");
    });

  }

  Atmos.prototype.search = function(){

    var filters = this.getFilters();

    console.log("Search started!", this.searchInput.val(), filters);

    console.log("Initiating query");

    this.clearResults();

    var scope = this;

    this.query(this.catalog, filters,
    function(interest, data){ //Response function
      console.log("Query Response:", interest, data);

      scope.name = data.getContent().toString().replace(/[\n\0]/g,"");

      scope.getResults(0);

    }, function(interest){ //Timeout function
      console.warn("Request failed after 3 attempts!", interest);
      scope.createAlert("Request failed after 3 attempts. \"" + interest.getName().toUri() + "\" See console for details.");
    });

  }

  Atmos.prototype.autoComplete = function(field, callback){

    var scope = this;

    this.query(this.catalog, {"?": field},
    function(interest, data){

      var name = new Name(data.getContent().toString().replace(/[\n\0]/g,""));

      var interest = new Interest(name);
      interest.setInterestLifetimeMilliseconds(1500);
      interest.setMustBeFresh(true);

      var count = 3;

      var run = function(){

        if (--count === 0){
          console.warn("Interest timed out!", interest);
          scope.createAlert("Request failed after 3 attempts. \"" + interest.getName().toUri() + "\" See console for details.");
          return;
        }

        scope.face.expressInterest(interest,
        function(interest, data){

          if (data.getContent().length !== 0){
            callback(JSON.parse(data.getContent().toString().replace(/[\n\0]/g, "")));
          } else {
            callback([]);
          }

        }, run);
      }

      run();

    }, function(interest){
      console.error("Request failed! Timeout", interest);
      scope.createAlert("Request failed after 3 attempts. \"" + interest.getName().toUri() + "\" See console for details.");
    });

  }

  Atmos.prototype.showResults = function(resultIndex) {

    var results = this.results.slice(this.resultsPerPage * resultIndex, this.resultsPerPage * (resultIndex + 1));

    var resultDOM = $(
      results.reduce(function(prev, current){
        prev.push('<tr><td><input class="resultSelector" type="checkbox"></td><td>');
        prev.push(current);
        prev.push('</td></tr>');
        return prev;
      }, ['<tr><th><input id="resultSelectAll" type="checkbox" title="Select All"> Select</th><th>Name</th></tr>']).join('')
    );

    resultDOM.find('#resultSelectAll').click(function(){
      if ($(this).is(':checked')){
        resultDOM.find('.resultSelector:not([disabled])').prop('checked', true);
      } else {
        resultDOM.find('.resultSelector:not([disabled])').prop('checked', false);
      }
    });

    this.resultTable.hide().empty().append(resultDOM).slideDown('slow');

    this.resultMenu.find('.pageNumber').text(resultIndex + 1);
    this.resultMenu.find('.pageLength').text(this.resultsPerPage * resultIndex + results.length);

    if (this.resultsPerPage * (resultIndex + 1) >= this.resultCount) {
      this.resultMenu.find('.next').addClass('disabled');
    } else if (resultIndex === 0){
      this.resultMenu.find('.next').removeClass('disabled');
    }

    if (resultIndex === 0){
      this.resultMenu.find('.previous').addClass('disabled');
    } else if (resultIndex === 1) {
      this.resultMenu.find('.previous').removeClass('disabled');
    }

  }

  Atmos.prototype.getResults = function(index){

    if ($('#results').hasClass('hidden')){
      $('#results').removeClass('hidden').slideDown();
    }

    $.scrollTo("#results", 500, {interrupt: true});

    if ((this.results.length === this.resultCount) || (this.resultsPerPage * (index + 1) < this.results.length)){
      //console.log("We already have index", index);
      this.page = index;
      this.showResults(index);
      return;
    }

    if (this.name === null) {
      console.error("This shouldn't be reached! We are getting results before a search has occured!");
      throw new Error("Illegal State");
    }

    var first = new Name(this.name).appendSegment(this.retrievedSegments++);

    console.log("Requesting data index: (", this.retrievedSegments - 1, ") at ", first.toUri());

    var scope = this;

    var interest = new Interest(first)
    interest.setInterestLifetimeMilliseconds(5000);
    interest.setMustBeFresh(true);

    this.face.expressInterest(interest,
      function(interest, data){ //Response

        if (data.getContent().length === 0){
          scope.resultMenu.find('.totalResults').text(0);
          scope.resultMenu.find('.pageNumber').text(0);
          scope.resultMenu.find('.pageLength').text(0);
          console.log("Empty response.");
          return;
        }

        var content = JSON.parse(data.getContent().toString().replace(/[\n\0]/g,""));

        if (!content.results){
          scope.resultMenu.find('.totalResults').text(0);
          scope.resultMenu.find('.pageNumber').text(0);
          scope.resultMenu.find('.pageLength').text(0);
          console.log("No results were found!");
          return;
        }

        scope.results = scope.results.concat(content.results);

        scope.resultCount = content.resultCount;

        scope.resultMenu.find('.totalResults').text(scope.resultCount);

        scope.page = index;

        scope.getResults(index); //Keep calling this until we have enough data.

      },
      function(interest){ //Timeout
        console.error("Failed to retrieve results: timeout", interest);
        scope.createAlert("Request timed out. \"" + interest.getName().toUri() + "\" See console for details.");
      }
    );

  }

  Atmos.prototype.query = function(prefix, parameters, callback, timeout) {

    var queryPrefix = new Name(prefix);
    queryPrefix.append("query");

    var jsonString = JSON.stringify(parameters);
    queryPrefix.append(jsonString);

    var queryInterest = new Interest(queryPrefix);
    queryInterest.setInterestLifetimeMilliseconds(1500);
    queryInterest.setMustBeFresh(true);

    var face = this.face;
    var retry = 3;
    var run = function(interest){
      if (--retry === 0){
        timeout(interest);
      } else {
        face.expressInterest(queryInterest, callback, run);
      }
    }
    run();

  }

  /**
   * This function returns a map of all the categories active filters.
   * @return {Object<string, string>}
   */
  Atmos.prototype.getFilters = function(){
    var filters = this.filters.children().toArray().reduce(function(prev, current){
      var data = $(current).text().split(/:/);
      prev[data[0]] = data[1];
      return prev;
    }, {}); //Collect a map<category, filter>.
    //TODO Make the return value map<category, Array<filter>>
    return filters;
  }

  /**
   * Creates a closable alert for the user.
   *
   * @param {string} message
   * @param {string} type - Override the alert type.
   */
  Atmos.prototype.createAlert = function(message, type) {

    var alert = $('<div class="alert"><div>');
    alert.addClass(type?type:'alert-info');
    alert.text(message);
    alert.append(closeButton);

    this.alerts.append(alert);
  }

  /**
   * Requests all of the names represented by the buttons in the elements list.
   *
   * @param elements {Array<jQuery>} A list of the table row elements
   */
  Atmos.prototype.request = function(){

    //Pseudo globals.
    var keyChain;
    var certificateName;
    var keyAdded = false;

    return function(elements){

      var names = [];
      var destination = $('#requestDest .active').text();
      $(elements).find('>*:nth-child(2)').each(function(){
        var name = $(this).text();
        names.push(name);
      });//.append('<span class="badge">Requested!</span>')
      //Disabling the checkbox doesn't make sense anymore with the ability to request to multiple destinations.
      //$(elements).find('.resultSelector').prop('disabled', true).prop('checked', false);

      var scope = this;
      this.requestForm.on('submit', function(e){ //This will be registered for the next submit from the form.
        e.preventDefault();

        //Form checking
        var dest = scope.requestForm.find('#requestDest .active');
        if (dest.length !== 1){
          $('#requestForm').append($('<div class="alert alert-warning">A destination is required!' + closeButton + '<div>'));
          return;
        }

        $('#request').modal('hide')//Initial params are ok. We can close the form.
        .remove('.alert') //Remove any alerts

        scope.cleanRequestForm();

        $(this).off(e); //Don't fire this again, the request must be regenerated

        //Key setup
        if (!keyAdded){
          if (!scope.config.retrieval.demoKey || !scope.config.retrieval.demoKey.pub || !scope.config.retrieval.demoKey.priv){
            scope.createAlert("This host was not configured to handle retrieval! See console for details.", 'alert-danger');
            console.error("Missing/invalid key! This must be configured in the config on the server.", scope.config.demoKey);
            return;
          }

          //FIXME base64 may or may not exist in other browsers. Need a new polyfill.
          var pub = new Buffer(base64.toByteArray(scope.config.retrieval.demoKey.pub)); //MUST be a Buffer (Buffer != Uint8Array)
          var priv = new Buffer(base64.toByteArray(scope.config.retrieval.demoKey.priv));

          var identityStorage = new MemoryIdentityStorage();
          var privateKeyStorage = new MemoryPrivateKeyStorage();
          keyChain = new KeyChain(new IdentityManager(identityStorage, privateKeyStorage),
                        new SelfVerifyPolicyManager(identityStorage));

          var keyName = new Name("/retrieve/DSK-123");
          certificateName = keyName.getSubName(0, keyName.size() - 1)
            .append("KEY").append(keyName.get(-1))
            .append("ID-CERT").append("0");

          identityStorage.addKey(keyName, KeyType.RSA, new Blob(pub, false));
          privateKeyStorage.setKeyPairForKeyName(keyName, KeyType.RSA, pub, priv);

          scope.face.setCommandSigningInfo(keyChain, certificateName);

          keyAdded = true;

        }

        //Retrieval
        var retrievePrefix = new Name("/catalog/ui/" + guid());

        scope.face.registerPrefix(retrievePrefix,
          function(prefix, interest, face, interestFilterId, filter){ //On Interest
            //This function will exist until the page exits but will likely only be used once.

            var data = new Data(interest.getName());
            var content = JSON.stringify(names);
            data.setContent(content);
            keyChain.sign(data, certificateName);

            try {
              face.putData(data);
              console.log("Responded for", interest.getName().toUri(), data);
              scope.createAlert("Data retrieval has initiated.", "alert-success");
            } catch (e) {
              console.error("Failed to respond to", interest.getName().toUri(), data);
              scope.createAlert("Data retrieval failed.");
            }

          }, function(prefix){ //On fail
            scope.createAlert("Failed to register the retrieval URI! See console for details.", "alert-danger");
            console.error("Failed to register URI:", prefix.toUri(), prefix);

          }, function(prefix, registeredPrefixId){ //On success
            var name = new Name(dest.text());
            name.append(prefix);
            var interest = new Interest(name);
            interest.setInterestLifetimeMilliseconds(1500);
            var count = 3;
            var run = function(i2){

              if (--count === 0) {
                console.error("Request for", name.toUri(), "timed out (3 times).", i2);
                scope.createAlert("Request for " + name.toUri() + " timed out after 3 attempts. This means that the retrieve failed! See console for more details.");
                return;
              }

              scope.face.expressInterest(interest,
                function(interest, data){ //Success
                  console.log("Request for", name.toUri(), "succeeded.", interest, data);
                },
                run
              );
            }
            run();
          }
        );

      });
      $('#request').modal(); //This forces the form to be the only option.

    }
  }();

  Atmos.prototype.filterSetup = function() {
    //Filter setup

    var prefix = new Name(this.catalog).append("filters-initialization");

    var scope = this;

    this.getAll(prefix, function(data) { //Success
      var raw = JSON.parse(data.replace(/[\n\0]/g, '')); //Remove null byte and parse

      console.log("Filter categories:", raw);

      $.each(raw, function(index, object){ //Unpack list of objects
        $.each(object, function(category, searchOptions) { //Unpack category from object (We don't know what it is called)
          //Create the category
          var e = $('<li><a href="#">' + category.replace(/_/g, " ") + '</a><ul class="subnav nav nav-pills nav-stacked"></ul></li>');

          var sub = e.find('ul.subnav');
          $.each(searchOptions, function(index, name){
            //Create the filter list inside the category
            var item = $('<li><a href="#">' + name + '</a></li>');
            sub.append(item);
            item.click(function(){ //Click on the side menu filters
              if (item.hasClass('active')){ //Does the filter already exist?
                item.removeClass('active');
                scope.filters.find(':contains(' + category + ':' + name + ')').remove();
              } else { //Add a filter
                item.addClass('active');
                var filter = $('<span class="label label-default"></span>');
                filter.text(category + ':' + name);

                scope.filters.append(filter);

                filter.click(function(){ //Click on a filter
                  filter.remove();
                  item.removeClass('active');
                });
              }

            });
          });

          //Toggle the menus. (Only respond when the immediate tab is clicked.)
          e.find('> a').click(function(){
            scope.categories.find('.subnav').slideUp();
            var t = $(this).siblings('.subnav');
            if ( !t.is(':visible') ){ //If the sub menu is not visible
              t.slideDown(function(){
                t.triggerHandler('focus');
              }); //Make it visible and look at it.
            }
          });

          scope.categories.append(e);

        });
      });

    }, function(interest){ //Timeout
      scope.createAlert("Failed to initialize the filters!", "alert-danger");
      console.error("Failed to initialize filters!", interest);
      ga('send', 'event', 'error', 'filters');
    });

  }

  /**
   * This function retrieves all segments in order until it knows it has reached the last one.
   * It then returns the final joined result.
   */
  Atmos.prototype.getAll = function(prefix, callback, timeout){

    var scope = this;
    var d = [];

    var count = 3;
    var retry = function(interest){
      if (count === 0){
        timeout(interest);
      } else {
        count--;
        request(interest.getName().get(-1).toSegment());
      }
    }

    var request = function(segment){

      var name = new Name(prefix);
      name.appendSegment(segment);

      var interest = new Interest(name);
      interest.setInterestLifetimeMilliseconds(1500);
      interest.setMustBeFresh(true); //Is this needed?

      scope.face.expressInterest(interest, handleData, retry);

    }

    var handleData = function(interest, data){

      d.push(data.getContent().toString());

      if (interest.getName().get(-1).toSegment() == data.getMetaInfo().getFinalBlockId().toSegment()){
        callback(d.join(""));
      } else {
        request(interest.getName().get(-1).toSegment() + 1);
      }

    }

    request(0);

  }

  Atmos.prototype.cleanRequestForm = function(){
    $('#requestDest').prev().removeClass('btn-success').addClass('btn-default');
    $('#requestDropText').text('Destination');
    $('#requestDest .active').removeClass('active');
  }

  Atmos.prototype.setupRequestForm = function(){

    var scope = this;

    this.requestForm.find('#requestCancel').click(function(){
      $('#request').unbind('submit') //Removes all event handlers.
      .modal('hide'); //Hides the form.
      scope.cleanRequestForm();
    });

    var dests = $(this.config['retrieval']['destinations'].reduce(function(prev, current){
      prev.push('<li><a href="#">');
      prev.push(current);
      prev.push("</a></li>");
      return prev;
    }, []).join(""));

    this.requestForm.find('#requestDest').append(dests)
    .on('click', 'a', function(e){
      $('#requestDest .active').removeClass('active');
      var t = $(this);
      t.parent().addClass('active');
      $('#requestDropText').text(t.text());
      $('#requestDest').prev().removeClass('btn-default').addClass('btn-success');
    });

    //This code will remain unused until users must use their own keys instead of the demo key.
//    var scope = this;

//    var warning = '<div class="alert alert-warning">' + closeButton + '<div>';

//    var handleFile = function(e){
//      var t = $(this);
//      if (e.target.files.length > 1){
//        var el = $(warning);
//        t.append(el.append("We are looking for a single file, we will try the first only!"));
//      } else if (e.target.files.length === 0) {
//        var el = $(warning.replace("alert-warning", "alert-danger"));
//        t.append(el.append("No file was supplied!"));
//        return;
//      }

//      var reader = new FileReader();
//      reader.onload = function(e){
//        var key;
//        try {
//          key = JSON.parse(e.target.result);
//        } catch (e) {
//          console.error("Could not parse the key! (", key, ")");
//          var el = $(warning.replace("alert-warning", "alert-danger"));
//          t.append(el.append("Failed to parse the key file, is it a valid json key?"));
//        }

//        if (!key.DEFAULT_RSA_PUBLIC_KEY_DER || !key.DEFAULT_RSA_PRIVATE_KEY_DER) {
//          console.warn("Invalid key", key);
//          var el = $(warning.replace("alert-warning", "alert-danger"));
//          t.append(el.append("Failed to parse the key file, it is missing required attributes."));
//        }


//      };

//    }

//    this.requestForm.find('#requestDrop').on('dragover', function(e){
//      e.dataTransfer.dropEffect = 'copy';
//    }).on('drop', handleFile);

//    this.requestForm.find('input[type=file]').change(handleFile);

  }

  return Atmos;

})();
