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
    this.page = 1;
    this.itemsPerPage = 25;

    this.catalog = catalog;

    this.face = new Face(config);
    this.categories = $('#side-menu');
    this.resultTable = $('#resultTable');
    this.filters = $('#filters');
    this.searchInput = $('#search');
    this.searchBar = $('#searchBar');

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

    this.searchBar.submit(function(e){
      e.preventDefault();
      scope.search();
    });

    this.searchInput.autoComplete(function(field, callback){
      scope.autoComplete(field, callback);
    });

  }

  Atmos.prototype.search = function(){
    var filters = this.getFilters();

    if (this.searchInput.val().length === 0 && !filters.hasOwnProperty()){
      if (!this.searchBar.hasClass('has-error')){
        this.searchBar.addClass('has-error').append('<span class="help-block">A filter or search value is required!</span>');
      }
      return;
    } else {
      this.searchBar.removeClass('has-error').find('.help-block').fadeOut(function(){$(this).remove()});
    }

    console.log("Search started!", this.searchInput.val(), filters);

    console.log("Initiating query"); 

    this.query(this.catalog, filters, 
    function(interest, data){ //Response function
      console.log("Query Response:", interest, data);

      

    }, function(interest){ //Timeout function
      console.error("Request failed! Timeout");
    });

  }

  Atmos.prototype.autoComplete = function(field, callback){
    console.log("Autocomplete triggered");

    var filters = this.getFilters();

    filters["?"] = this.searchInput.val();

    this.query(this.catalog, filters,
    function(interest, data){
      console.log(interest, data);
    }, function(interest){
      console.error("Request failed! Timeout");
    });

  }

  Atmos.prototype.onData = function(data) {
    console.log("Recieved data", data);

    var payloadStr = data.content.toString().split("\n")[0];

    if (!payloadStr || payloadStr.length === 0){
      this.populateResults();
      return; //No results were returned.
    }

    var queryResults = JSON.parse(payloadStr);

    var scope = this;

    $.each(queryResults, function (queryResult, field) {

      if (queryResult == "next") {
        scope.populateAutocomplete(field);
      }

      if (queryResult == "results" && field == null){
        return; //Sometimes the results are null. (We should skip this.)
      }

      $.each(field, function (entryCount, name) {
        scope.results.push(name);
      });
    });

    // Calculating the current page and the view
    this.populateResults();

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

  Atmos.prototype.expressNextInterest = function() {
    // @todo pipelines
    var nextName = new Name(this.state["results"]);
    nextName.appendSegment(this.state["nextSegment"]);

    var nextInterest = new Interest(nextName);
    nextInterest.setInterestLifetimeMilliseconds(10000);

    var scope = this;

    this.face.expressInterest(nextInterest,
        function(interest, data){
          scope.onQueryResultsData(interest, data);
        },
        function(interest){
          scope.onQueryResultsTimeout(interest);
        });

    this.state["nextSegment"] ++;
    this.state["outstanding"][nextName.toUri()] = 0;
  }

  Atmos.prototype.onQueryData = function(interest, data) {
    var name = data.getName();

    delete this.state["outstanding"][interest.getName().toUri()];

    this.state["version"] = name.get(this.state["prefix"].size() + 2).toVersion();

    this.state["results"] = new Name(this.state["prefix"]).append("query-results").append(this.state['parameters'])
    .appendVersion(this.state["version"]).append(name.getComponent(name.getComponentCount() - 2));

    console.log("Requested URI", this.state.results.toUri());

    this.expressNextInterest();
  }

  Atmos.prototype.onQueryResultsData = function(interest, data) {
    var name = data.getName();
    delete this.state["outstanding"][interest.getName().toUri()];
    if (!name.get(-1).equals(data.getMetaInfo().getFinalBlockId())) {
      this.expressNextInterest();
    } //else {
    //alert("found final block");
    //}

    this.state["userOnData"](data);
  }

  Atmos.prototype.onQueryTimeout = function(interest) {
    var uri = interest.getName().toUri();
    if (this.state["outstanding"][uri] < 1) {
      this.state["outstanding"][uri] ++;
      var scope = this;
      this.face.expressInterest(interest,
          function(interest, data){
            scope.onQueryData(interest, data);
          },
          function(interest){
            scope.onQueryTimeout(interest);
          });
    } else {
      delete this.state["outstanding"][uri];

      // We modify the autocomplete box here because we need to know
      // we have all of the entries first. Fairly hacky.
      /* TODO FIXME
         var autocompleteFullName = this.autocompleteText.value;
         for (var i = 0; i < dropdown.length; ++i) {
         if (this.dropdown[i].substr(0, dropdown[i].length - 1).toUpperCase === this.autocompleteText.value.toUpperCase || dropdown.length == 1) {
         autocompleteText.value = dropdown[i];
         }
         }
         */
    }
  }

  Atmos.prototype.onQueryResultsTimeout = function(interest) {
    var uri = interest.getName().toUri();
    if (this.state["outstanding"][uri] < 1) {
      this.state["outstanding"][uri] ++;
      var scope = this;
      this.face.expressInterest(interest,
          function(){
            scope.onQueryResultsData.apply(scope, arguments);
          },
          function(){
            scope.onQueryResultsTimeout.apply(scope, arguments);
          });
    } else {
      delete this.state["outstanding"][uri];
      // We modify the autocomplete box here because we need to know
      // we have all of the entries first. Fairly hacky.
      /* TODO FIXME
         var autocompleteFullName = autocompleteText.value;
         for (var i = 0; i < dropdown.length; ++i) {
         if (dropdown[i].substr(0, dropdown[i].length - 1).toUpperCase === autocompleteText.value.toUpperCase || dropdown.length == 1) {
         autocompleteText.value = dropdown[i];
         }
         }
         */
    }
  }

  Atmos.prototype.populateResults = function() {

    //TODO Check only for page changes and result length

    this.resultTable.empty();

    for (var i = startIndex; i < startIndex + 20 && i < this.results.length; ++i) {
      this.resultTable.append('<tr><td>' + this.results[i]
          + '</td><td><button class="interest-button btn btn-primary btn-xs">Retrieve</button></td></tr>');
    }

    if (this.results.length <= 20) {
      this.page = 1;
    } else {
      this.page = startIndex / 20 + 1;
    }

    this.totalPages = Math.ceil(this.results.length / 20);

    //TODO Fix the page to fit the theme.
    var currentPage = $(".page");
    currentPage.empty();
    if (this.page != 1) {
      currentPage.append('<a href="#" onclick="getPage(this.id);" id="<"><</a>');
    }
    // This section of code creates the paging for the results.
    // To prevent it from having a 1000+ pages, it will only show the 5 pages before/after
    // the current page and the total pages (expect users not to really jump around a lot).
    for (var i = 1; i <= this.totalPages; ++i) {
      if (i == 1 || i == this.totalPages      // Min or max
          || (i <= this.page && i + 5 >= this.page)    // in our current page range
          || (i >= this.page && i - 5 <= this.page)) { // in our current page range
        if (i != this.page) {
          currentPage.append(' <a href="#" onclick="getPage(' + i + ');">' + i + '</a>');
            if (i == 1 && this.page > i + 5) {
              currentPage.append(' ... ');
            }
        } else {
          currentPage.append(' ' + i);
        }
      } else { // Need to skip ahead
        if (i == this.page + 6) {
          currentPage.append(' ... ');

          currentPage.append(' <a href="#" onclick="getPage(this.id);" id=">">></a>')
            i = this.totalPages - 1;
        }
      }
    }
    currentPage.append('  ' + this.results.length + ' results');
  }

  Atmos.prototype.getPage = function(clickedPage) {
    console.log(clickedPage);

    var nextPage = clickedPage;
    if (clickedPage === "<") {
      nextPage = this.page - 5;
    } else if (clickedPage === ">") {
      console.log("> enabled");

      nextPage = this.page + 5;
    }

    nextPage--; // Need to adjust for starting at 0

    if (nextPage < 0 ) {
      nextPage = 0;
      console.log("0 enabled");
    } else if (nextPage > this.totalPages - 1) {
      nextPage = this.totalPages - 1;
      console.log("total enabled");
    }

    this.populateResults(nextPage * 20);
    return false;
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

  return Atmos;

})();


