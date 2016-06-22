/*
 * The following code is a jquery extention written to add autocomplete functionality to bootstrap input groups.
 *
 * Usage:
 *
 * Then simply call $('.someClass').autoComplete(getSuggestions) on it to enable auto completion.
 *
 * getSuggestions returns by calling its callback parameter with an array of valid strings.
 *
 * Autocomplete can be manually triggered by triggering the autoComplete event.
 *
 */
(function(){
  "use strict";
  if (!jQuery){
    throw new Error("jQuery is required and must be loaded before this script.")
  }
  jQuery.fn.extend({
    /**
     * This setups up the default autocomplete functionality on an object.
     * Using either a list of options or supplying a function that returns them,
     * auto complete will create a menu for users to navigate and select a
     * valid option.
     * @param {Array<String>|function(String, function(Array<String>))} suggestions
     */
    autoComplete: function(suggestions) {

      var element = $('<div></div>');
      element.addClass('list-group')
      .addClass('autoComplete')
      .css({
        'top': this.parent().height()
      });

      this.attr('autocomplete', 'off')
      .after(element);

      var getSuggestions = function(current, callback){
        callback(suggestions.reduce(function(prev, suggestion){
          if (current.toLowerCase().indexOf(suggestion.substr(0, current.length).toLowerCase()) === 0){
            prev.push(suggestion);
          }
          return prev;
        }, []));
      }

      var lastList = [];

      var setAutoComplete = function(list){
        lastList = list;
        element.empty();

        element.html(list.reduce(function(prev, current){
          return [prev, '<a href="#" class="list-group-item">', current, '</a>'].join("");
        }, ""));

      }

      if (suggestions instanceof Function){
        getSuggestions = suggestions;
      }

      var input = this;

      var matcher = /^\/([-_\w]+\/)*/; //Returns only the absolute path.

      var getValue = function(){
        var res = matcher.exec(input.val());
        if (res){
          return res[0]; //Return the absolute match
        } else {
          throw new Error("Empty or incorrectly formatted path.");
        }
      }

      element.bind('click', 'a', function(){
        input.val($(event.target).text());
        getSuggestions(getValue(), setAutoComplete);
        input.focus();
      });

      var updateFromList = function(){
        var val = input.val(); //Needs to be unfiltered, for filtering existing results.
        var temp = lastList;
        setAutoComplete(lastList.reduce(function(prev, current){
          if (current.indexOf(val) === 0){
            prev.push(current);
          }
          return prev;
        }, []));
        lastList = temp;
      }

      this.keydown(function(e){
        switch(e.which){
          case 38: //up
          var active = element.find('.active');
          if (active.length === 0){
            element.find(':first-child').addClass('active');
          } else {
            if (!active.is(':first-child')){
              var top = active.removeClass('active').prev().addClass('active').offset().top;
              active.parent().stop().animate({scrollTop: top}, 500);
            }
          }
          e.preventDefault();
          break;

          case 40: //down
          var active = element.find('.active');
          if (active.length === 0){
            element.find(':first-child').addClass('active');
          } else {
            if (!active.is(':last-child')){
              var top = active.removeClass('active').next().addClass('active').offset().top;
              active.parent().stop().animate({scrollTop: top}, 500);
            }
          }
          e.preventDefault();
          break;

          case 13: //Enter
          var active = element.find('.active');
          if (active.length === 1){
            $(this).val(active.text());
            e.preventDefault();
            getSuggestions(getValue(), setAutoComplete);
          }
          break;

          case 9: //Tab
          getSuggestions(getValue(), setAutoComplete);
          e.preventDefault(); //Don't print tab or select a different element.
          break;

          case 8:
          console.log("Detected backspace");
          if (input.val().slice(-1) == "/"){
            e.preventDefault();
            input.val(input.val().slice(0,-1)); //Manually backspace early. (Have to do it manually)
            getSuggestions(getValue(), setAutoComplete);
          } else {
            updateFromList(setAutoComplete);
          }

          default:
          updateFromList();

        }

      })
      .keyup(function(e){
        switch (e.which){

          case 191:
          getSuggestions(getValue(), setAutoComplete);

          break;
          case 38:
          case 40:
          case 13:
          case 9:
          // Do nothing

          break;
          default:
          updateFromList();
        }
      });

      this.on('autoComplete', function(){
        getSuggestions(getValue(), setAutoComplete);
      });

      return this;

    }
  });
})();

