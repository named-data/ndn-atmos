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
    this.pagers = $('.pager');
    this.alerts = $('#alerts');

    var scope = this;

    this.resultTable.on('click', '.interest-button', function(){
      var button = $(this);

      if (button.is(':disabled')){
        console.warn("Attempt to request again!");
      }

      var name = button.parent().prev().text();
      var interest = new Interest(new Name('/retrieve' + name));
      scope.face.expressInterest(interest, function(){}, function(){});

      button.text("Requested!")
        .removeClass('btn-primary')
        .addClass('btn-success')
        .addClass('disabled')
        .prop('disabled', true);
    });

    //Filter setup
    $.getJSON("search_catagories.json").done(function (data) {
      $.each(data, function (pageSection, contents) {
        if (pageSection == "SearchCatagories") {
          $.each(contents, function (category, searchOptions) {
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
        }
      });
    });

    this.searchInput.autoComplete(function(field, callback){
      scope.autoComplete(field, callback);
    });

    this.searchBar.submit(function(e){
      e.preventDefault();
      scope.createAlert("This feature is currently incomplete.");
    });

    this.searchButton.click(function(){
      console.log("Search Button Pressed");
      scope.search();
    });

    this.pagers.find('.next').click(function(){
      if (!$(this).hasClass('disabled')){
        scope.getResults(scope.page + 1);
      }
    });
    this.pagers.find('.previous').click(function(){
      if (!$(this).hasClass('disabled')){
        scope.getResults(scope.page - 1);
      }
    });
    this.pagers.find('.pageLength').attr('contentEditable', true)
    .blur(function(){
      scope.resultsPerPage = Number($(this).text());
      scope.pagers.find('.pageLength').text(scope.resultsPerPage);
      scope.getResults(0); //Reset page to 0;
    });

  }

  Atmos.prototype.search = function(){

    var filters = this.getFilters();

    console.log("Search started!", this.searchInput.val(), filters);

    console.log("Initiating query");

    this.results = []; //Drop any old results.
    this.retrievedSegments = 0;
    this.resultCount = Infinity;
    this.page = 0;
    this.resultTable.empty();

    var scope = this;

    this.query(this.catalog, filters,
    function(interest, data){ //Response function
      console.log("Query Response:", interest, data);

      var parameters = JSON.stringify(filters);

      var ack = data.getName();

      scope.name = new Name(scope.catalog).append("query-results").append(parameters).append(ack.get(-3)).append(ack.get(-2));

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

      var ack = data.getName();

      var name = new Name(scope.catalog).append('query-results').append(JSON.stringify({"?": field})).append(ack.get(-3)).append(ack.get(-2));

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

    var results = this.results.slice(this.resultsPerPage * resultIndex, this.resultsPerPage * (resultIndex + 1));

    var resultDOM = $(results.reduce(function(prev, current){
      prev.push('<tr><td><input type="checkbox"></td><td>');
      prev.push(current);
      prev.push('</td><td><button class="interest-button btn btn-primary btn-sm">Retrieve</button></td></tr>');
      return prev;
    }, []).join(''));

    this.resultTable.empty().append(resultDOM);

    this.pagers.find('.pageNumber').text(resultIndex + 1);

    if (this.resultsPerPage * (resultIndex + 1) >= this.resultCount) {
      this.pagers.find('.next').addClass('disabled');
    } else if (resultIndex === 0){
      this.pagers.find('.next').removeClass('disabled');
    }

    if (resultIndex === 0){
      this.pagers.find('.previous').addClass('disabled');
    } else if (resultIndex === 1) {
      this.pagers.find('.previous').removeClass('disabled');
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
          scope.pagers.find('.totalResults').text(0);
          scope.pagers.find('.pageNumber').text(0);
          console.log("Empty response.");
          return;
        }

        var content = JSON.parse(data.getContent().toString().replace(/[\n\0]/g,""));

        if (!content.results){
          scope.pagers.find('.totalResults').text(0);
          scope.pagers.find('.pageNumber').text(0);
          console.log("No results were found!");
          return;
        }

        scope.results = scope.results.concat(content.results);

        scope.resultCount = content.resultCount;

        scope.pagers.find('.totalResults').text(scope.resultCount);

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
  Atmos.closeButton = '<button type="button" class="close" data-dismiss="alert" aria-label="Close"><span aria-hidden="true">&times;</span></button>';

  return Atmos;

})();
