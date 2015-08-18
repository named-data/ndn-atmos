//Run when the document loads AND we have the config loaded.
(function(){
  var catalog = null;
  var config = null;
  Promise.all([
    new Promise(function(resolve, reject){
      $.ajax('config.json').done(function(data){
        catalog = data.catalogPrefix;
        config = data.faceConfig;
        resolve();
      }).fail(function(){
        console.error("Failed to get config.");
        reject()
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
    new Atmos(catalog, config);
  }, function(){
    console.error("Failed to initialize!");
  })
})();

var Atmos = (function(){
  "use strict";
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
  function Atmos(catalog, config){

    //Internal variables.
    this.results = [];
    this.resultCount = Infinity;
    this.name = null;
    this.page = 0;
    this.resultsPerPage = 25;
    this.retrievedSegments = 0;

    this.catalog = catalog;

    this.face = new Face(config);

    this.categories = $('#side-menu');
    this.resultTable = $('#resultTable');
    this.filters = $('#filters');
    this.searchInput = $('#search');
    this.searchBar = $('#searchBar');
    this.searchButton = $('#searchButton');
    this.resultMenu = $('.resultMenu');
    this.alerts = $('#alerts');

    var scope = this;

    this.resultTable.on('click', '.interest-button', function(){
      scope.request(this);
    });

    $('.requestSelectedButton').click(function(){
      scope.request(
        scope.resultTable.find('.resultSelector:checked:not([disabled])')
        .parent().next().find('.interest-button')
      );
    });

    this.filterSetup();

    this.searchInput.autoComplete(function(field, callback){
      scope.autoComplete(field, callback);
    });

    this.searchBar.submit(function(e){
      e.preventDefault();
      scope.pathSearch();
    });

    this.searchButton.click(function(){
      console.log("Search Button Pressed");
      scope.search();
    });

    this.resultMenu.find('.next').click(function(){
      if (!$(this).hasClass('disabled')){
        scope.getResults(scope.page + 1);
      }
    });
    this.resultMenu.find('.previous').click(function(){
      if (!$(this).hasClass('disabled')){
        scope.getResults(scope.page - 1);
      }
    });

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
      console.warn("Request failed! Timeout");
      scope.createAlert("Request timed out. \"" + interest.getName().toUri() + "\" See console for details.");
    });

  }

  Atmos.prototype.autoComplete = function(field, callback){

    if (this.searchInput.val().length === 0 && !filters.hasOwnProperty()){
      if (!this.searchBar.hasClass('has-error')){
        this.searchBar.addClass('has-error').append('<span class="help-block">A filter or search value is required!</span>');
      }
      return;
    } else {
      this.searchBar.removeClass('has-error').find('.help-block').fadeOut(function(){$(this).remove()});
    }

    var scope = this;

    this.query(this.catalog, {"?": field},
    function(interest, data){

      var name = new Name(data.getContent().toString().replace(/[\n\0]/g,""));

      var interest = new Interest(name);
      interest.setInterestLifetimeMilliseconds(5000);
      interest.setMustBeFresh(true);

      scope.face.expressInterest(interest,
      function(interest, data){

        if (data.getContent().length !== 0){
          var options = JSON.parse(data.getContent().toString().replace(/[\n\0]/g, "")).next.map(function(element){
            return field + element + "/";
          });
          callback(options);
        }

      }, function(interest){
        console.warn("Interest timed out!", interest);
        scope.createAlert("Request timed out. \"" + interest.getName().toUri() + "\" See console for details.");
      });

    }, function(interest){
      console.error("Request failed! Timeout", interest);
      scope.createAlert("Request timed out. \"" + interest.getName().toUri() + "\" See console for details.");
    });

  }

  Atmos.prototype.showResults = function(resultIndex) {

    if ($('#results').hasClass('hidden')){
      $('#results').removeClass('hidden').slideDown();
    }

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

    this.resultTable.empty().append(resultDOM).slideDown();
    if (this.resultMenu.hasClass('hidden')){
      this.resultMenu.removeClass('hidden').slideDown();
    }

    this.resultMenu.find('.pageNumber').text(resultIndex + 1);
    this.resultMenu.find('.pageLength').text(this.resultsPerPage * (resultIndex + 1));

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
          console.log("Empty response.");
          return;
        }

        var content = JSON.parse(data.getContent().toString().replace(/[\n\0]/g,""));

        if (!content.results){
          scope.resultMenu.find('.totalResults').text(0);
          scope.resultMenu.find('.pageNumber').text(0);
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
    queryInterest.setInterestLifetimeMilliseconds(4000);
    queryInterest.setMustBeFresh(true);

    this.face.expressInterest(queryInterest, callback, timeout);

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
    alert.append(Atmos.closeButton);

    this.alerts.append(alert);
  }

  /**
   * Requests all of the names represented by the buttons in the elements list.
   *
   * @param elements {Array<jQuery>} A list of the interestButton elements
   */
  Atmos.prototype.request = function(elements){

    var scope = this;
    $(elements).filter(':not(.disabled)').each(function(){
      var button = $(this);

      if (button.hasClass('disabled')){
        console.warn("An attempt to request a disabled element has occured");
        return;
      }

      var name = button.text();
      var interest = new Interest(new Name('/retrieve' + name));
      scope.face.expressInterest(interest, function(){}, function(){});

    })
    .append('<span class="badge">Requested!</span>')
    .addClass('disabled')
    .addClass('label-success')
    .parent().prev().find('.resultSelector').prop('disabled', true).prop('checked', false);

  }

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
          var e = $('<li><a href="#">' + category.replace(/\_/g, " ") + '</a><ul class="subnav nav nav-pills nav-stacked"></ul></li>');

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
    });

  }

  Atmos.prototype.getAll = function(prefix, callback, timeout){

    var scope = this;
    var d = [];

    var request = function(segment){

      var name = new Name(prefix);
      name.appendSegment(segment);

      var interest = new Interest(name);
      interest.setInterestLifetimeMilliseconds(1000);
      interest.setMustBeFresh(true); //Is this needed?

      scope.face.expressInterest(interest, handleData, timeout);

    }


    var handleData = function(interest, data){

      d.push(data.getContent().toString());

      if (interest.getName().get(-1).toSegment() == data.getMetaInfo().getFinalBlockId().toSegment()){
        callback(d.join(""));
      } else {
        request(interest.getName().toSegment()++);
      }

    }

    request(0);


  }

  Atmos.closeButton = '<button type="button" class="close" data-dismiss="alert" aria-label="Close"><span aria-hidden="true">&times;</span></button>';

  return Atmos;

})();
