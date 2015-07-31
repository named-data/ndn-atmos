var catalog = "/catalog/myUniqueName";
var config = {
  host: "atmos-csu.research-lan.colostate.edu",
  port: 9696
};

//Run when the document loads.
$(function () {
  new Atmos(catalog, config);
});

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
    this.resultCount = 0;
    this.name = null;
    this.page = 0;
    this.lastPage = -1;
    //this.itemsPerPage = 25; //TODO

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
        .addClass('btn-default')
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
      console.warn("This feature is incomplete.");
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

  }

  Atmos.prototype.search = function(){

    var filters = this.getFilters();

    console.log("Search started!", this.searchInput.val(), filters);

    console.log("Initiating query");

    this.results = []; //Drop any old results.
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
    console.log("Autocomplete triggered");

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

      var name = new Name(scope.catalog).append('query-results').append(JSON.stringify(filters)).append(ack.get(-3)).append(ack.get(-2));

      console.log(name.toUri(), filters);

      scope.face.expressInterest(new Interest(name).setInterestLifetimeMilliseconds(5000),
      function(interest, data){
        console.log("Autocomplete query return: ", data.getContent().toString());

        if (data.getContent().length !== 0){
          var options = JSON.parse(data.getContent().toString().replace(/[\n\0]/g, "")).next.map(function(element){
            return field + element;
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

    var results = $('<tr><td>' + this.results[resultIndex].join('</td><td><button class="interest-button btn btn-primary btn-sm">Retrieve</button></td></tr><tr><td>') +
    '</td><td><button class="interest-button btn btn-primary btn-sm">Retrieve</button></td></tr>'); //Fastest way to generate the table.

    this.resultTable.empty().append(results);

    this.pagers.find('.totalResults').text('(Page' + (resultIndex + 1) + ') Showing ' + this.results[resultIndex].length + ' of ' + this.resultCount + ' results');

    if (resultIndex === this.lastPage) {
      this.pagers.find('.next').addClass('disabled');
    }

    if (resultIndex === 0){
      this.pagers.find('.next').removeClass('disabled');
      this.pagers.find('.previous').addClass('disabled');
    } else if (resultIndex === 1) {
      this.pagers.find('.previous').removeClass('disabled');
    }

  }

  Atmos.prototype.getResults = function(index){

    if (this.results[index]){
      //console.log("We already have index", index);
      this.page = index;
      this.showResults(index);
      return;
    }

    if (this.name === null) {
      console.error("This shouldn't be reached! We are getting results before a search has occured!");
      throw new Error("Illegal State");
    }

    var first = new Name(this.name).appendSegment(index);

    console.log("Requesting data index: (", index, ") at ", first.toUri());

    var scope = this;

    this.face.expressInterest(new Interest(first).setInterestLifetimeMilliseconds(5000),
      function(interest, data){ //Response

        if (data.getContent().length === 0){
          console.log("Empty response.");
          return;
        }

        if (data.getName().get(-1).equals(data.getMetaInfo().getFinalBlockId())) { //Final page.
          scope.lastPage = index;
          //The next buttons will be disabled by showResults.
        }

        var content = JSON.parse(data.getContent().toString().replace(/[\n\0]/g,""));

        var results = scope.results[index] = content.results;

        scope.resultCount = content.resultCount;

        scope.pagers.find('.totalResults').text(scope.resultCount + " Results");

        //console.log("Got results:", results);

        scope.page = index;

        if (!results){
          console.log("No results were found!");
          return;
        }

        scope.showResults(index);

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


