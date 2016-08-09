//Run when the document loads AND we have the config loaded.
(function() {
  "use strict";
  var config;
  var conversions;

  Promise.all([new Promise(function(resolve, reject) {
    $.ajax('config.json').done(function(data) {
      config = data;
      resolve();
    }).fail(function() {
      console.error("Failed to get config.");
      ga('send', 'event', 'error', 'config');
      reject();
    });
  }), new Promise(function(resolve, reject) {
    var timeout = setTimeout(function() {
      console.error("Document never loaded? Something bad has happened!");
      reject();
    }, 10000);
    $(function() {
      clearTimeout(timeout);
      resolve();
    });
  }), new Promise(function(resolve, reject) {
    $.getJSON('../conversions.json').done(function(data) {
      conversions = data;
      resolve();
    }).fail(function() {
      console.error("Failed to get conversions.");
      ga('send', 'event', 'error', 'config');
      //reject(); We will continue anyways. We don't need this functionality.
      conversions = {};
      resolve();
    });
  })]).then(function() {
    var getParameterByName = function(name) {
      name = name.replace(/[\[]/, "\\[").replace(/[\]]/, "\\]");
      var regex = new RegExp("[\\?&]" + name + "=([^&#]*)");
      var results = regex.exec(location.search);
      return results === null ? "" : decodeURIComponent(results[1].replace(/\+/g, " "));
    }
    //Overwrite config if present. Any failure will just cause this to be skipped.
    try {
      var configParam = JSON.parse(getParameterByName('config'));
      config = jQuery.extend(true, config, configParam);
    } catch (e) {
      console.warn("Failure in config overwrite, skipping.", e);
    }
    new Atmos(config,conversions);
  }, function() {
    console.error("Failed to initialize!");
    ga('send', 'event', 'error', 'init');
  });

})();
var Atmos = (function() {
  "use strict";
  var closeButton = '<button type="button" class="close" data-dismiss="alert" aria-label="Close"><span aria-hidden="true">&times;</span></button>';
  var guid = function() {
    var d = new Date().getTime();
    var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
      var r = (d + Math.random() * 16) % 16 | 0;
      d = Math.floor(d / 16);
      return (c == 'x' ? r : (r & 0x3 | 0x8)).toString(16);
    });
    return uuid;
  }
  /**
   * Atmos
   * @version 2.0

   * Configures an Atmos object. This manages the atmos interface.

   * @constructor
   * @param {string} catalog - NDN path
   * @param {Object} config - Object of configuration options for a Face.
   */
  var Atmos = function(config, conversions) {
    //Internal variables.
    this.results = [];
    this.resultCount = Infinity;
    this.name = null ;
    this.page = 0;
    this.resultsPerPage = 25;
    this.retrievedSegments = 0;

    //Config/init
    this.config = config;
    this.conversions = conversions;

    this.catalog = config['global']['catalogPrefix'];
    this.catalogPrefix = new Name(this.catalog);
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
    $('.requestSelectedButton').click(function() {
      ga('send', 'event', 'button', 'click', 'request');
      scope.request(scope.resultTable.find('.resultSelector:checked:not([disabled])').parent().parent());
    });
    this.filterSetup();

    //Init autocomplete
    this.searchInput.autoComplete(function(field, callback) {
      ga('send', 'event', 'search', 'autocomplete');
      scope.autoComplete(field, function(data) {
        var list = data.next;
        var last = data.lastComponent === true;
        callback(list.map(function(element) {
          return field + element + (last ? "/" : "");
          //Don't add trailing slash for last component.
        }));
      });
    });

    //Handle search
    this.searchBar.submit(function(e) {
      ga('send', 'event', 'search', 'submit');
      e.preventDefault();
      if (scope.searchInput.val().length === 0) {
        if (!scope.searchBar.hasClass('has-error')) {
          scope.searchBar.addClass('has-error').append('<span class="help-block">Search path is required!</span>');
        }
        return;
      } else {
        scope.searchBar.removeClass('has-error').find('.help-block').fadeOut(function() {
          $(this).remove()
        });
      }
      scope.pathSearch();
    });

    this.searchButton.click(function() {
      console.log("Search Button Pressed");
      ga('send', 'event', 'button', 'click', 'search');
      scope.search();
    });

    //Result navigation handlers
    this.resultMenu.find('.next').click(function() {
      ga('send', 'event', 'button', 'click', 'next');
      if (!$(this).hasClass('disabled')) {
        scope.getResults(scope.page + 1);
      }
    });

    this.resultMenu.find('.previous').click(function() {
      ga('send', 'event', 'button', 'click', 'previous');
      if (!$(this).hasClass('disabled')) {
        scope.getResults(scope.page - 1);
      }
    });

    this.resultMenu.find('.clearResults').click(function() {
      ga('send', 'event', 'button', 'click', 'resultClear');
      scope.clearResults();
      $('#results').fadeOut(function() {
        $(this).addClass('hidden');
      });
    });

    //Change the number of results per page handler
    var rpps = $('.resultsPerPageSelector').click(function() {
      var t = $(this);
      if (t.hasClass('active')) {
        return;
      }
      rpps.find('.active').removeClass('active');
      t.addClass('active');
      scope.resultsPerPage = Number(t.text());
      scope.getResults(0);
      //Force return to page 1;
    });

    //Init tree search
    $('#treeSearch div').treeExplorer(function(path, callback) {
      console.log("Tree Explorer request", path);
      ga('send', 'event', 'tree', 'request');
      scope.autoComplete(path, function(data) {
        var list = data.next;
        var last = (data.lastComponent === true);
        if (last) {
          console.log("Redirecting last element request to a search.");
          scope.clearResults();
          scope.query(scope.catalog, {
            '??': path
          }, function(interest, data) {
            console.log("Search response", interest, data);
            scope.name = interest.getName();
            scope.getResults(0);
          }, function(interest) {
            console.warn("Failed to retrieve final component.", interest, path);
            scope.createAlert("Failed to request final component. " + path + " See console for details.");
          });
          return;
          //Don't call the callback
        }
        console.log("Autocomplete response", list);
        callback(list.map(function(element) {
          return (path == "/" ? "/" : "") + element + "/";
        }));
      });
    });

    $('#treeSearch').on('click', '.treeSearch', function() {
      var t = $(this);
      scope.clearResults();
      var path = t.parent().parent().attr('id');
      console.log("Tree search:", path);
      scope.query(scope.catalog, {
        '??': path
      }, function(interest, data) {
        //Success
        console.log("Tree search response", interest, data);
        scope.name = interest.getName();
        scope.getResults(0);
      }, function(interest) {
        //Failure
        console.warn("Request failed! Timeout", interest);
        scope.createAlert("Request timed out.\"" + interest.getName().toUri() + "\" See console for details.");
      });
    });

    this.setupRequestForm();
    this.resultTable.popover({
      selector: ".metaDataLink",
      content: function() {
        $('.metaDataLink').not(this).popover('destroy');
        return scope.getMetaData(this);
      },
      title: "Metadata",
      html: true,
      trigger: 'click',
      placement: 'bottom'
    });

    this.resultTable.on('click', '.metaDataLink', function(e) {
      //This prevents the page from scrolling when you click on a name.
      e.preventDefault();
    });

    this.resultTable.on('click', '.subsetButton', function() {
      var metaData = $(this).siblings('pre').text();
      var exp = /netcdf ([\w-]+)/;
      var match = exp.exec(metaData);
      var filename = match[0].replace(/netcdf /, '') + '.nc';
      scope.request(null , filename);
    });

    //Allow the title to change the tab
    $('#brand-title').click(function() {
      //Correct active class on tabs.
      $('#path-search-tab').removeClass('active');
      $('#tree-search-tab').removeClass('active');
      $('#search-tab').addClass('active');
    });
  }
  Atmos.prototype.clearResults = function() {
    this.results = [];
    //Drop any old results.
    this.retrievedSegments = 0;
    this.resultCount = Infinity;
    this.page = 0;
    this.resultTable.empty();
  }

  Atmos.prototype.pathSearch = function() {
    var value = this.searchInput.val();
    this.clearResults();
    var scope = this;
    this.query(this.catalog, {
      "??": value
    }, function(interest, data) {
      console.log("Query response:", interest, data);
      scope.name = interest.getName();
      scope.getResults(0);
    }, function(interest) {
      console.warn("Request failed! Timeout", interest);
      scope.createAlert("Request timed out. \"" + interest.getName().toUri() + "\" See console for details.");
    });
  }

  Atmos.prototype.search = function() {
    var filters = this.getFilters();
    console.log("Search started!", this.searchInput.val(), filters);
    console.log("Initiating query");
    this.clearResults();
    var scope = this;
    this.query(this.catalog, filters, function(interest, data) {
      //Response function
      console.log("Query Response:", interest, data);
      scope.name = interest.getName();
      scope.getResults(0);
    }, function(interest) {
      //Timeout function
      console.warn("Request failed after 3 attempts!", interest);
      scope.createAlert("Request failed after 3 attempts. \"" + interest.getName().toUri() + "\" See console for details.");
    });
  }

  Atmos.prototype.autoComplete = function(field, callback) {
    var scope = this;
    var result = {};
    const getAll = function(interest, data) {
      if (data.getContent().length !== 0) {
        var resp = JSON.parse(data.getContent().toString().replace(/[\n\0]/g, ""));
        if (result.next) {
          result.next = result.next.concat(resp.next);
        } else {
          result = resp;
        }
      } else {
        callback(result);
      }
      var name = data.getName();
      var segment = name.components[name.getComponentCount() - 1];
      if (segment.toSegment() !== data.getMetaInfo().getFinalBlockId().toSegment()) {
        name = name.getPrefix(-1);
        //Remove segment
        name.appendSegment(segment.toSegment() + 1);
        scope.expressInterest(name, getAll, function() {
          console.warn("Autocomplete timed out, results may be incomplete.");
          callback(result);
          //Return if we get a timeout.
        });
      } else {
        callback(result);
      }
    }
    this.query(this.catalog, {
      "?": field
    }, getAll);
  }

  Atmos.prototype.showResults = function(resultIndex) {
    var results = this.results.slice(this.resultsPerPage * resultIndex, this.resultsPerPage * (resultIndex + 1));
    var resultDOM = $(results.reduce(function(prev, current) {
      prev.push('<tr><td><input class="resultSelector" type="checkbox"></td><td class="popover-container">');
      if (current.has_metadata){
        prev.push('<a href="#" class="metaDataLink">');
      }
      prev.push(current.name);
      if (current.has_metadata){
        prev.push('</a>');
      }
      prev.push('</td></tr>');
      return prev;
    }, ['<tr><th><input id="resultSelectAll" type="checkbox"> Select All</th><th>Name</th></tr>']).join(''));
    resultDOM.find('#resultSelectAll').click(function() {
      if ($(this).is(':checked')) {
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
    } else if (resultIndex === 0) {
      this.resultMenu.find('.next').removeClass('disabled');
    }
    if (resultIndex === 0) {
      this.resultMenu.find('.previous').addClass('disabled');
    } else if (resultIndex === 1) {
      this.resultMenu.find('.previous').removeClass('disabled');
    }
    $.scrollTo("#results", 500, {
      interrupt: true
    });
  }

  Atmos.prototype.getResults = function(index) {

    var scope = this;

    if ($('#results').hasClass('hidden')) {
      $('#results').removeClass('hidden').slideDown();
    }

    if ((scope.results.length === scope.resultCount) || (scope.resultsPerPage * (index + 1) < scope.results.length)) {
      //console.log("We already have index", index);
      scope.page = index;
      scope.showResults(index);
      return;
    }

    if (scope.name === null ) {
      console.error("This shouldn't be reached! We are getting results before a search has occured!");
      throw new Error("Illegal State");
    }

    var interestName = new Name(scope.name);
    // Interest name should be /<catalog-prefix>/query/<query-param>/<version>/<#seq>
    if (scope.name.size() === (scope.catalogPrefix.size() + 3)) {
      interestName = interestName.appendSegment(scope.retrievedSegments++);
      //console.log("Requesting data index: (", scope.retrievedSegments - 1, ") at ", interestName.toUri());
    }

    this.expressInterest(interestName, function(interest, data) {
      //Response
      if (data.getContent().length === 0) {
        scope.resultMenu.find('.totalResults').text(0);
        scope.resultMenu.find('.pageNumber').text(0);
        scope.resultMenu.find('.pageLength').text(0);
        console.log("Empty response.");
        scope.resultTable.html("<tr><td>Empty response. This usually means no results.</td></tr>");
        return;
      }

      var content = JSON.parse(data.getContent().toString().replace(/[\n\0]/g, ""));

      if (!content.results) {
        scope.resultMenu.find('.totalResults').text(0);
        scope.resultMenu.find('.pageNumber').text(0);
        scope.resultMenu.find('.pageLength').text(0);
        console.log("No results were found!");
        scope.resultTable.html("<tr><td>No Results</td></tr>");
        return;
      }

      scope.results = scope.results.concat(content.results);
      scope.resultCount = content.resultCount;
      scope.resultMenu.find('.totalResults').text(scope.resultCount);
      scope.page = index;
      // reset scope.name
      scope.name = new Name(data.getName().getPrefix(scope.catalogPrefix.size() + 3));
      scope.getResults(index);
      //Keep calling this until we have enough data.
    }, function() {});//Ignore failure

  }
  Atmos.prototype.query = function(prefix, parameters, callback, timeout) {
    var queryPrefix = new Name(prefix);
    queryPrefix.append("query");
    var jsonString = JSON.stringify(parameters);
    queryPrefix.append(jsonString);
    this.expressInterest(queryPrefix, callback, timeout);
  }

  Atmos.prototype.expressInterest = function(name, success, failure) {
    var interest = new Interest(name);
    interest.setInterestLifetimeMilliseconds(500);
    interest.setMustBeFresh(true);
    const face = this.face;
    async.retry(4, function(done) {
      face.expressInterest(interest, function(interest, data) {
        done();
        success(interest, data);
      }, function(interest) {
        done("Interest timed out 4 times.", interest);
      });
    }, function(err, interest) {
      if (err) {
        console.log(err, interest);
        failure(interest);
      }
    });
  }

  /**
   * This function returns a map of all the categories active filters.
   * @return {Object<string, string>}
   */
  Atmos.prototype.getFilters = function() {
    var filters = this.filters.children().toArray().reduce(function(prev, current) {
      var data = $(current).text().split(/:/);
      prev[data[0]] = data[1];
      return prev;
    }, {});
    //Collect a map<category, filter>.
    //TODO Make the return value map<category, Array<filter>>
    return filters;
  }

  /**
   * Creates a closable alert for the user.

   * @param {string} message
   * @param {string} type - Override the alert type.
   */
  Atmos.prototype.createAlert = function(message, type) {
    var alert = $('<div class="alert"><div>');
    alert.addClass(type ? type : 'alert-info');
    alert.text(message);
    alert.append(closeButton);
    this.alerts.append(alert);
  }

  /**
   * Requests all of the names represented by the buttons in the elements list.

   * @param elements {Array<jQuery>} A list of the table row elements
   * @param subsetFileName {String} If present then do a subsetting request instead.
   */
  Atmos.prototype.request = function() {
    //Pseudo globals.
    var keyChain;
    var certificateName;
    var keyAdded = false;
    return function(elements, subsetFilename) {

      var names = [];

      $(elements).find('.metaDataLink').each(function() {
        var name = $(this).text();
        names.push(name);
      });
      var subset = false;
      if (!subsetFilename) {
        $('#subsetting').hide();
      } else {
        $('#subsetting').show();
        subset = true;
      }

      var scope = this;

      //FIXME The following is temporary, it allows people to direct download from
      //a single host with a small set of names. It is to demo the functionality but
      //could use improvement. (Multiple servers, non static list, etc)
      var directDls = $('#directDownloadList').empty();
      names.forEach(function(name) {
        if (scope.conversions[name]) {
          //If the name exists in the conversions.
          var ele = $('<a href="http://atmos-mwsc.ucar.edu/ucar/' + conversions[name] + '" class="list-group-item>' + name + '</a>');
          directDls.append(ele);
        }
      });

      this.requestForm.on('submit', function(e) {
        //This will be registered for the next submit from the form.
        e.preventDefault();
        $('#request .alert').remove();
        var variables = [];
        if (subset) {
          $('#subsetVariables .row').each(function() {
            var t = $(this);
            var values = {};
            t.find('.values input').each(function() {
              var t = $(this);
              values[t.attr('name')] = t.val();
            });
            variables.push({
              variable: t.find('.variable').val(),
              values: values
            });
          });
        }

        //Form checking
        var dest = scope.requestForm.find('#requestDest .active');
        if (dest.length !== 1) {
          var alert = $('<div class="alert alert-warning">A destination is required!' + closeButton + '<div>');
          $('#request > .panel-body').append(alert);
          return;
        }

        $('#request').modal('hide');

        //Initial params are ok. We can close the form.
        scope.cleanRequestForm();

        $(this).off(e); //Don't fire this again, the request must be regenerated

        //Key setup
        if (!keyAdded) {
          if (!scope.config.retrieval.demoKey || !scope.config.retrieval.demoKey.pub || !scope.config.retrieval.demoKey.priv) {
            scope.createAlert("This host was not configured to handle retrieval! See console for details.", 'alert-danger');
            console.error("Missing/invalid key! This must be configured in the config on the server.", scope.config.demoKey);
            return;
          }
          //FIXME base64 may or may not exist in other browsers. Need a new polyfill.
          var pub = new Buffer(base64.toByteArray(scope.config.retrieval.demoKey.pub));
          //MUST be a Buffer (Buffer != Uint8Array)
          var priv = new Buffer(base64.toByteArray(scope.config.retrieval.demoKey.priv));
          var identityStorage = new MemoryIdentityStorage();
          var privateKeyStorage = new MemoryPrivateKeyStorage();
          keyChain = new KeyChain(new IdentityManager(identityStorage,privateKeyStorage),new SelfVerifyPolicyManager(identityStorage));
          var keyName = new Name("/retrieve/DSK-123");
          certificateName = keyName.getSubName(0, keyName.size() - 1).append("KEY").append(keyName.get(-1)).append("ID-CERT").append("0");
          identityStorage.addKey(keyName, KeyType.RSA, new Blob(pub,false));
          privateKeyStorage.setKeyPairForKeyName(keyName, KeyType.RSA, pub, priv);
          scope.face.setCommandSigningInfo(keyChain, certificateName);
          keyAdded = true;
        }

        //Retrieval
        var retrievePrefix = new Name("/catalog/ui/" + guid());
        scope.face.registerPrefix(retrievePrefix, function(prefix, interest, face, interestFilterId, filter) {
          //On Interest
          //This function will exist until the page exits but will likely only be used once.
          var data = new Data(interest.getName());
          var content;
          if (subset) {
            content = JSON.stringify({
              name: subsetFilename,
              subset: variables
            });
          } else {
            content = JSON.stringify(names);
          }

          //Blob breaks the data! Don't use it
          data.setContent(content);

          //TODO Packetize this.
          keyChain.sign(data, certificateName);

          try {
            face.putData(data);
            console.log("Responded for", interest.getName().toUri(), data);
            scope.createAlert("Data retrieval has initiated.", "alert-success");
          } catch (e) {
            console.error("Failed to respond to", interest.getName().toUri(), data);
            scope.createAlert("Data retrieval failed.");
          }
        }, function(prefix) {
          //On fail
          scope.createAlert("Failed to register the retrieval URI! See console for details.", "alert-danger");
          console.error("Failed to register URI:", prefix.toUri(), prefix);
        }, function(prefix, registeredPrefixId) {
          //On success
          var name = new Name(dest.text());
          name.append(prefix);
          scope.expressInterest(name, function(interest, data) {
            //Success
            console.log("Request for", name.toUri(), "succeeded.", interest, data);
          }, function() {
            console.warn("Failed to request from retrieve agent.");
          });
        });
      });
      $('#request').modal();
      //This forces the form to be the only option.
    }
  }();

  Atmos.prototype.filterSetup = function() {
    //Filter setup
    var prefix = new Name(this.catalog).append("filters-initialization");

    var scope = this;

    this.getAll(prefix, function(data) {
      //Success
      var raw = JSON.parse(data.replace(/[\n\0]/g, ''));
      //Remove null byte and parse
      console.log("Filter categories:", raw);

      $.each(raw, function(index, object) {
        //Unpack list of objects

        $.each(object, function(category, searchOptions) {
          //Unpack category from object (We don't know what it is called)
          //Create the category
          var e = $('<li><a href="#">' + category.replace(/_/g, " ") + '</a><ul class="subnav nav nav-pills nav-stacked"></ul></li>');
          var sub = e.find('ul.subnav');
          $.each(searchOptions, function(index, name) {
            //Create the filter list inside the category

            var item = $('<li><a href="#">' + name + '</a></li>');
            sub.append(item);
            item.click(function() {
              //Click on the side menu filters
              if (item.hasClass('active')) {
                //Does the filter already exist?
                item.removeClass('active');
                scope.filters.find(':contains(' + category + ':' + name + ')').remove();
              } else {
                //Add a filter
                item.addClass('active');
                var filter = $('<span class="label label-default"></span>');
                filter.text(category + ':' + name);
                scope.filters.append(filter);
                filter.click(function() {
                  //Click on a filter
                  filter.remove();
                  item.removeClass('active');
                });
              }
            });
          });
          //Toggle the menus. (Only respond when the immediate tab is clicked.)
          e.find('> a').click(function() {
            scope.categories.find('.subnav').slideUp();
            var t = $(this).siblings('.subnav');
            if (!t.is(':visible')) {
              //If the sub menu is not visible
              t.slideDown(function() {
                t.triggerHandler('focus');
              });
              //Make it visible and look at it.
            }
          });
          scope.categories.append(e);
        });
      });
    }, function(interest) {
      //Timeout
      scope.createAlert("Failed to initialize the filters!", "alert-danger");
      console.error("Failed to initialize filters!", interest);
      ga('send', 'event', 'error', 'filters');
    });
  }

  /**
   * This function retrieves all segments in order until it knows it has reached the last one.
   * It then returns the final joined result.

   * @param prefix {String|Name} The ndn name we are retrieving.
   * @param callback {function(String)} if successful, will call the callback with a string of data.
   * @param failure {function(Interest)} if unsuccessful, will call failure with the last failed interest.
   * @param stop {boolean} stop if no finalBlock.
   */
  Atmos.prototype.getAll = function(prefix, callback, failure, stop) {
    var scope = this;
    var d = [];
    var name = new Name(prefix);
    var segment = 0;
    var request = function() {
      var n2 = new Name(name);
      n2.appendSegment(segment);
      scope.expressInterest(n2, handleData, function(err, interest) {
        failure(interest)
      });
      //Forward to handleData and ignore error
    }
    var handleData = function(interest, data) {
      d.push(data.getContent().toString());
      var hasFinalBlock = data.getMetaInfo().getFinalBlockId().value.length === 0;
      var finalBlockStop = hasFinalBlock && stop;
      if (finalBlockStop || (!hasFinalBlock && interest.getName().get(-1).toSegment() == data.getMetaInfo().getFinalBlockId().toSegment())) {
        callback(d.join(""));
      } else {
        segment++;
        request();
      }
    }
    request();
  }

  Atmos.prototype.cleanRequestForm = function() {
    $('#requestDest').prev().removeClass('btn-success').addClass('btn-default');
    $('#requestDropText').text('Destination');
    $('#requestDest .active').removeClass('active');
    $('#subsetMenu').attr('class', 'collapse');
    $('#subsetVariables').empty();
    $('#request .alert').alert('close').remove();
  }

  Atmos.prototype.setupRequestForm = function() {
    var scope = this;
    this.requestForm.find('#requestCancel').click(function() {
      $('#request').unbind('submit')//Removes all event handlers.
      .modal('hide');
      //Hides the form.
      scope.cleanRequestForm();
    });
    var dests = $(this.config['retrieval']['destinations'].reduce(function(prev, current) {
      prev.push('<li><a href="#">');
      prev.push(current);
      prev.push("</a></li>");
      return prev;
    }, []).join(""));
    this.requestForm.find('#requestDest').append(dests).on('click', 'a', function(e) {
      $('#requestDest .active').removeClass('active');
      var t = $(this);
      t.parent().addClass('active');
      $('#requestDropText').text(t.text());
      $('#requestDest').prev().removeClass('btn-default').addClass('btn-success');
    });
    var addVariable = function(selector) {
      var ele = $(selector).clone().attr('id', '');
      ele.find('.close').click(function() {
        ele.remove();
      });
      $('#subsetVariables').append(ele);
    }
    $('#subsetAddVariableBtn').click(function() {
      addVariable('#customTemplate');
    });
    $('#subsetAddTimeVariable').click(function() {
      addVariable('#timeTemplate');
    });
    $('#subsetAddLocVariable').click(function() {
      addVariable('#locationTemplate');
    });
  }

  Atmos.prototype.getMetaData = (function() {
    var cache = {};
    return function(element) {
      var name = $(element).text();
      ga('send', 'event', 'request', 'metaData');
      var subsetButton = '<button class="btn btn-default subsetButton" type="button">Subset</button>';
      if (cache[name]) {
        return [subsetButton, '<pre class="metaData">', cache[name], '</pre>'].join('');
      }
      var prefix = new Name(name).append("metadata");
      var id = guid();
      //We need an id because the return MUST be a string.
      var ret = '<div id="' + id + '"><span class="fa fa-spinner fa-spin"></span></div>';
      this.getAll(prefix, function(data) {
        var el = $('<pre class="metaData"></pre>');
        el.text(data);
        var container = $('<div></div>');
        container.append($(subsetButton));
        container.append(el);
        $('#' + id).empty().append(container);
        cache[name] = data;
      }, function(interest) {
        $('#' + id).text("The metadata is unavailable for this name.");
        console.log("Data is unavailable for " + name);
      });
      return ret;
    }
  })();

  return Atmos;

})();
