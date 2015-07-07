//{@ @todo: this need to be configured before the document load
var catalog = "/catalog/myUniqueName";
var config = {
  host: "atmos-csu.research-lan.colostate.edu",
  port: 9696
};

// @}

var atmos = {}; //Comment this out if you don't want debug access.

//Run when the document loads.
$(function () {
  
  //remove "atmos =" if you don't want debug access
  atmos = new Atmos(catalog, config);
  
});

/*
  Atmos
  Version 2
*/

function Atmos(catalog, config){
  "use strict";
  //Internal variables.
  this.results = []
  this.resultCount = 0;
  this.page = 1;
  this.totalPages = 1;
  this.selectedSearch = {};
  this.dropdown = [];
  this.state = {};
  this.currentViewIndex = 0;

  this.face = new Face(config);
  this.categories = $('#side-menu');
  this.resultTable = $('#resultTable');

  var scope = this;

  this.resultTable.on('click', '.interest-button', function(){
    var button = $(this);

    var name = button.parent().prev().text();
    var interest = new Interest(new Name('/retrieve' + name));
    scope.face.expressInterest(interest, function(){
      var message = $('<div class="success"><span class="glyphicon glyphicon-ok"></span> Success!</div>');
      message.insertAfter(button);
      message.fadeOut(5000);
    }, function(){
      var message = $('<div class="fail"><span class="glyphicon glyphicon-remove"></span> Failed!</div>');
      message.insertAfter(button);
      message.fadeOut(5000);
    });

  });

  $.getJSON("search_catagories.json").done(function (data) {
    $.each(data, function (pageSection, contents) {
      if (pageSection == "SearchCatagories") {
        $.each(contents, function (search, searchOptions) {
          var e = $('<li><a href="#">' + search.replace(/\_/g, " ") + '</a><ul class="subnav nav nav-pills nav-stacked"></ul></li>');

          var sub = e.find('ul.subnav');
          $.each(searchOptions, function(index, name){
            var item = $('<li><a href="#">' + name + '</a></li>');
            sub.append(item);
            item.click(function(){
              scope.submitCatalogSearch(name);
            });
          });

          //Toggle the menus.
          e.click(function(){
            scope.categories.find('.subnav').slideUp();
            var t = $(this).find('.subnav');
            if ( !t.is(':visible')){
              t.slideDown().triggerHandler('focus'); //Cancel other animations and slide down.
            }
          });

          scope.categories.append(e);
        });
      }
    });
  });

  $('#searchBar').submit(function(e){
    e.preventDefault();
  })

}

Atmos.prototype.onData = function(data) {
  var payloadStr = data.content.toString().split("\n")[0];

  var queryResults = JSON.parse(payloadStr);

  var scope = this;

  $.each(this.queryResults, function (queryResult, field) {

    if (queryResult == "next") {
      scope.populateAutocomplete(field);
    }

    $.each(field, function (entryCount, name) {
      scope.results.push(name);
    });
  });

  // Calculating the current page and the view
  this.totalPages = Math.ceil(this.resultCount / 20);
  this.populateResults(0);
}

Atmos.prototype.query = function(prefix, parameters, callback, pipeline) {
  this.results = [];
  this.dropdown = [];
  this.resultTable.empty();
  this.resultTable.append('<tr><th colspan="2">Results</th></tr>');

  var queryPrefix = new Name(prefix);
  queryPrefix.append("query");

  var jsonString = JSON.stringify(parameters);
  queryPrefix.append(jsonString);

  this.state = {
      prefix: new Name(prefix),
      userOnData: callback,
      outstanding: {},
      nextSegment: 0,
  };

  /*if (state.hasOwnProperty("version")) {
                console.log("state already has version");
            }*/

  var queryInterest = new Interest(queryPrefix);
  queryInterest.setInterestLifetimeMilliseconds(10000);

  var scope = this;

  this.face.expressInterest(queryInterest,
    function(){ //FIXME
      scope.onQueryData.apply(scope, arguments); //TODO
    }, function(){
      scope.onQueryTimeout.apply(scope, arguments);
    }
  );

  this.state["outstanding"][queryInterest.getName().toUri()] = 0;
}

Atmos.prototype.submitCatalogSearch = function(field) {
  console.log("Sumbit Catalog Search: " + field);
  // @todo: this logic isn't quite right
  var remove = false;
  var scope = this;
  $.each(scope.selectedSearch, function (search, f) {
    if (field == f) {
      delete scope.selectedSearch[field];
      remove = true;
    }
  });
  if (!remove) {
    $.each(scope.searchMenuOptions, function (search, fields) {
      $.each(fields, function (index, f) {
        if (f == field) {
          scope.selectedSearch[search] = field;
        }
      });
    });
  }
  this.query(scope.catalog, scope.selectedSearch, function(){
    scope.onData.apply(scope, arguments); //Unknown arguments. FIXME (Works but could be improved for readability)
  }, 1);
  scope.populateCurrentSelections();
  return false;//?? Is this used?
}

Atmos.prototype.expressNextInterest = function() {
  // @todo pipelines
  var nextName = new Name(state["results"]);
  nextName.appendSegment(state["nextSegment"]);

  var nextInterest = new Interest(nextName);
  nextInterest.setInterestLifetimeMilliseconds(10000);

  var scope = this;

  this.face.expressInterest(nextInterest,
      function(){
        scope.onQueryResultsData.apply(scope, arguments); //FIXME
      },
      function(){
        scope.onQueryResultsTimeout.apply(scope, arguments); //FIXME
      });

  this.state["nextSegment"] ++;
  this.state["outstanding"][nextName.toUri()] = 0;
}

Atmos.prototype.onQueryData = function(interest, data) {
  var name = data.getName();

  delete this.state["outstanding"][interest.getName().toUri()];

  this.state["version"] = name.get(this.state["prefix"].size() + 2).toVersion();

  this.state["results"] = new Name(this.state["prefix"]).append("query-results").appendVersion(this.state["version"]);

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
        function(){
          scope.onQueryData.apply(scope, arguments);
        },
        function(){
          scope.onQueryTimeout.apply(scope, arguments);
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

Atmos.prototype.populateResults = function(startIndex) {
  this.resultTable.empty();
  this.resultTable.append('<tr><th colspan="2">Results</th></tr>');


  for (var i = startIndex; i < startIndex + 20 && i < this.results.length; ++i) {
    resultTable.append('<tr><td>' + this.results[i]
    + '</td><td><button class="interest-button btn btn-default btn-xs">Retrieve</button></td></tr>');
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
        currentPage.append(' <a href="#" onclick="getPage(' + i + ');">' + i + '</a>')
        if (i == 1 && this.page > i + 5) {
          currentPage.append(' ... ');
        }
      } else {
        currentPage.append(' ' + i);
      }
    } else { // Need to skip ahead
      if (i == page + 6) {
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


Atmos.prototype.submitAutoComplete = function() {
  /* FIXME TODO
  if (autocompleteText.value.length > 0) {
    var selection = autocompleteText.value;
    $.each(dropdown, function (i, dropdownEntry) {
      if (dropdownEntry.substr(0, dropdownEntry.length - 1) == selection) {
        selection = dropdownEntry;
      }
    });

    selectedSearch["?"] = selection;
    query(catalog, selectedSearch, onData, 1);
    delete selectedSearch["?"];
  }
  */
}

Atmos.prototype.populateAutocomplete = function(fields) {
  /* FIXME TODO
  var isAutocompleteFullName = (autocompleteText.value.charAt(autocompleteText.value.length - 1) === "/");
  var autocompleteFullName = autocompleteText.value;
  for (var i = 0; i < fields.length; ++i) {
    var fieldFullName = fields[i];
    var entry = autocompleteFullName;
    var skipahead = "";

    if (isAutocompleteFullName) {
      skipahead = fieldFullName.substr(autocompleteText.value.length, fieldFullName.length);
    } else {
      if (fieldFullName.charAt(autocompleteText.value.length) === "/") {
        entry += "/";
        skipahead = fieldFullName.substr(autocompleteText.value.length + 1, fieldFullName.length);
      } else {
        skipahead = fieldFullName.substr(autocompleteText.value.length, fieldFullName.length);
      }
    }
    if (skipahead.indexOf("/") != -1) {
      entry += skipahead.substr(0, skipahead.indexOf("/") + 1);
    } else {
      entry += skipahead;
    }

    var added = false;
    for (var j = 0; j < dropdown.length && !added; ++j) {
      if (dropdown[j] === entry) {
        added = true;
      } else if (dropdown[j] > entry) {
        dropdown.splice(j, 0, entry);
        added = true;
      }
    }
    if (!added) {
      dropdown.push(entry);
    }

  }
  $("#autocompleteText").autocomplete({
    source: dropdown
  });
  */
}

Atmos.prototype.populateCurrentSelections = function() { //TODO
  var currentSelection = $(".currentSelections");
  currentSelection.empty();

  currentSelection.append("<p>Filtering on:");

  var scope = this;

  $.each(this.selectedSearch, function (searchMenuCatagory, selection) {
    var e = $('<a href="#">[X] ' + searchMenuCatagory + ":" + selection + '</a>');
    e.onclick(function(){
      var searchFilter = $(this).text();

      var search = "";
      for (var j = 0; j < searchFilter.length; ++j) {
        search += searchFilter[j] + " ";
      }
      console.log("Split values: '" + search + "'");

      delete this.selectedSearch[searchFilter[0]];
      this.query(catalog, selectedSearch, onData, 1);
      populateCurrentSelections();
    });
    currentSelection.append(e);
  });

  currentSelection.append("</p>");
}
