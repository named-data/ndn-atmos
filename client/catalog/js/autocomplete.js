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
     * @param {Array<String>|getSuggestions}
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
        input.val($(this).text());
      });

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
          }
          break;

          case 9: //Tab
          getSuggestions(getValue(), setAutoComplete);
          e.preventDefault(); //Don't print tab or select a different element.
          break;

          default:
          var val = input.val(); //Needs to be unfiltered, for filtering existing results.
          setAutoComplete(lastList.reduce(function(prev, current){
            if (current.indexOf(val) === 0){
              prev.push(current);
            }
            return prev;
          }, []));

        }

      })
      .keyup(function(e){
        if (e.which === 191){
          getSuggestions(getValue(), setAutoComplete);
        }
      });

      this.on('autoComplete', function(){
        getSuggestions(getValue(), setAutoComplete);
      });

      return this;

    }
  });
})();

/**
 * @callback getSuggestions
 * @param {string} current - The current value of the input field.
 * @param {function}
 */
