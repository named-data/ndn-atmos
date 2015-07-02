//{@ @todo: this need to be configured before the document load
var catalog = "/catalog/myUniqueName";
var config = {
  host: "atmos-csu.research-lan.colostate.edu",
  port: 9696
};

// @}

var Atmos = {}; //Placeholder for the class.

(function(){
  "use strict"; //Magic (If you really wanna know, look it up.)

  function Atmos(catalog, config){
    //Internal variables.
    this.searchMenuOptions = {}
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
            search = search.replace(/\_/g, " ");

            searchMenu.append('<li id="' + search + '" onclick="getDropDown(this.id)"><a href="#">' + search + '</a></li>');
            scope.searchMenuOptions[String(search)] = searchOptions;
          });
        }
      });
    });

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
      function(){
        onQueryData()
      }, function(){
        onQueryTimeout()
      }
    );

    this.state["outstanding"][queryInterest.getName().toUri()] = 0;
  }

})(); //Run as soon as the script loads but keep the "use strict"" from escaping to the global space.


var atmos = {}; //Comment this out if you don't want debug access.

//Run when the document loads.
$(function () {

  atmos = new Atmos(catalog, config);
  
});

function expressNextInterest() {
  // @todo pipelines
  var nextName = new Name(state["results"]);
  nextName.appendSegment(state["nextSegment"]);

  var nextInterest = new Interest(nextName);
  nextInterest.setInterestLifetimeMilliseconds(10000);

  face.expressInterest(nextInterest,
      onQueryResultsData,
      onQueryResultsTimeout);

  state["nextSegment"] ++;
  state["outstanding"][nextName.toUri()] = 0;
}

function onQueryData(interest, data) {
  var name = data.getName();

  delete state["outstanding"][interest.getName().toUri()];

  state["version"] = name.get(state["prefix"].size() + 2).toVersion();

  state["results"] = new Name(state["prefix"]).append("query-results").appendVersion(state["version"]);

  expressNextInterest();
}

function onQueryResultsData(interest, data) {
  var name = data.getName();
  delete state["outstanding"][interest.getName().toUri()];
  if (!name.get(-1).equals(data.getMetaInfo().getFinalBlockId())) {
    expressNextInterest();
  } //else {
    //alert("found final block");
  //}

  state["userOnData"](data);
}

function onQueryTimeout(interest) {
  var uri = interest.getName().toUri();
  if (state["outstanding"][uri] < 1) {
    state["outstanding"][uri] ++;
    face.expressInterest(interest,
        onQueryData,
        onQueryTimeout);
  } else {
    delete state["outstanding"][uri];

    // We modify the autocomplete box here because we need to know
    // we have all of the entries first. Fairly hacky.
    var autocompleteFullName = autocompleteText.value;
    for (var i = 0; i < dropdown.length; ++i) {
      if (dropdown[i].substr(0, dropdown[i].length - 1).toUpperCase === autocompleteText.value.toUpperCase || dropdown.length == 1) {
        autocompleteText.value = dropdown[i];
      }
    }
  }
}

function onQueryResultsTimeout(interest) {
  var uri = interest.getName().toUri();
  if (state["outstanding"][uri] < 1) {
    state["outstanding"][uri] ++;
    face.expressInterest(interest,
        onQueryResultsData,
        onQueryResultsTimeout);
  } else {
    delete state["outstanding"][uri];
    // We modify the autocomplete box here because we need to know
    // we have all of the entries first. Fairly hacky.
    var autocompleteFullName = autocompleteText.value;
    for (var i = 0; i < dropdown.length; ++i) {
      if (dropdown[i].substr(0, dropdown[i].length - 1).toUpperCase === autocompleteText.value.toUpperCase || dropdown.length == 1) {
        autocompleteText.value = dropdown[i];
      }
    }
  }
}




function populateResults(startIndex) {
  var resultTable = $(".resultTable");
  resultTable.empty();
  resultTable.append('<tr><th colspan="2">Results</th></tr>');


  for (var i = startIndex; i < startIndex + 20 && i < results.length; ++i) {
    resultTable.append('<tr><td>' + results[i]
    + '</td><td><button class="interest-button btn btn-default btn-xs">Express Interest</button></td></tr>');
  }

  if (results.length <= 20) {
    page = 1;
  } else {
    page = startIndex / 20 + 1;
  }

  totalPages = Math.ceil(results.length / 20);

  var currentPage = $(".page");
  currentPage.empty();
  if (page != 1) {
    currentPage.append('<a href="#" onclick="getPage(this.id);" id="<"><</a>');
  }
  // This section of code creates the paging for the results.
  // To prevent it from having a 1000+ pages, it will only show the 5 pages before/after
  // the current page and the total pages (expect users not to really jump around a lot).
  for (var i = 1; i <= totalPages; ++i) {
    if (i == 1 || i == totalPages     // Min or max
        || (i <= page && i + 5 >= page)    // in our current page range
        || (i >= page && i - 5 <= page)) { // in our current page range
      if (i != page) {
        currentPage.append(' <a href="#" onclick="getPage(' + i + ');">' + i + '</a>')
        if (i == 1 && page > i + 5) {
          currentPage.append(' ... ');
        }
      } else {
        currentPage.append(' ' + i);
      }
    } else { // Need to skip ahead
      if (i == page + 6) {
        currentPage.append(' ... ');

        currentPage.append(' <a href="#" onclick="getPage(this.id);" id=">">></a>')
        i = totalPages - 1;
      }
    }
  }
  currentPage.append('  ' + results.length + ' results');
}

var dropState = "";

function getDropDown(str) {
  var searchMenu = $("#side-menu");
  if (str == dropState) {
    dropState = "";
    searchMenu.find("#" + str).find("#options_" + str).empty();
  } else {
    dropState = str;

    $.each(searchMenuOptions, function (search, fields) {
      if (search === str) {
        searchMenu.find("#" + search).append('<ul id="options_' + search + '" class="sub-menu">');
        for (var i = 0; i < fields.length; ++i) {
          searchMenu.find("#options_" + search).append('<li id="' + fields[i] + '" onclick="submitCatalogSearch(this.id)"><a href="#">' + fields[i] + '</a></li>');
        }
        searchMenu.append('</ul>');
      } else {
        var ul = $("options_" + search);
        ul.empty();
        searchMenu.find("#" + search).find("#options_" + search).empty();
      }
    });
  }
}

function getPage(clickedPage) {
  console.log(clickedPage);

  var nextPage = clickedPage;
  if (clickedPage === "<") {
    nextPage = page - 5;
  } else if (clickedPage === ">") {
    console.log("> enabled");

    nextPage = page + 5;
  }

  nextPage--; // Need to adjust for starting at 0

  if (nextPage < 0 ) {
    nextPage = 0;
    console.log("0 enabled");
  } else if (nextPage > totalPages - 1) {
    nextPage = totalPages - 1;
    console.log("total enabled");
  }

  populateResults(nextPage * 20);
  return false;
}

function submitAutoComplete() {
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
}

function submitCatalogSearch(field) {
  console.log("Sumbit Catalog Search: " + field);
  // @todo: this logic isn't quite right
  var remove = false;
  $.each(selectedSearch, function (search, f) {
    if (field == f) {
      delete selectedSearch[field];
      remove = true;
    }
  });
  if (!remove) {
    $.each(searchMenuOptions, function (search, fields) {
      $.each(fields, function (index, f) {
        if (f == field) {
          selectedSearch[search] = field;
        }
      });
    });
  }
  query(catalog, selectedSearch, onData, 1);
  populateCurrentSelections();
  return false;
}

function populateAutocomplete(fields) {
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
}

function populateCurrentSelections() {
  var currentSelection = $(".currentSelections");
  currentSelection.empty();

  currentSelection.append("<p>Filtering on:");

  $.each(selectedSearch, function (searchMenuCatagory, selection) {
    currentSelection.append('  <a href="#" onclick="removeFilter(this.id);" id="' + searchMenuCatagory + ':' + selection + '">[X] ' + searchMenuCatagory + ":" + selection + '</a>');
  });

  currentSelection.append("</p>");
}


function removeFilter(filter) {
  console.log("Remove filter" + filter);
  var searchFilter = filter.split(":");

  var search = "";
  for (var j = 0; j < searchFilter.length; ++j) {
    search += searchFilter[j] + " ";
  }
  console.log("Split values: '" + search + "'");

  delete selectedSearch[searchFilter[0]];
  query(catalog, selectedSearch, onData, 1);
  populateCurrentSelections();

  return false;
}