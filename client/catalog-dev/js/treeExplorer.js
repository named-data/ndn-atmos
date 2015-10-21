
(function(){
  "use strict";
  jQuery.fn.extend({

    treeExplorer: function(getChildren, settings){

      this.settings = {
        autoScroll : false
      }

      for (var value in settings) {
        if (this.settings[value] !== undefined){
          this.settings[value] = settings[value];
        }
      }

      var cache = {}; //Cache previously requested paths.

      var tree = $('<div class="treeExplorer"></div>');
      this.append(tree);

      var lookup = function(path, callback){
        if (cache[path]){
          callback(path, cache[path]);
        } else {
          getChildren(path, function(children){
            cache[path] = children;
            callback(path, children);
          });
        }
      }

      var append = function(path, children, node){
        var c = $('<div class="nodeChildren"></div>');
        node.append(c);
        children.forEach(function(current){
          var el = $('<div class="treeExplorerNode"></div>');
          if (current.match(/\/$/)){
            el.attr('id', path + current);
            el.append(['<div class="nodeContent"><a class="treeExplorerExpander"></a><a class="treeSearch" href="#', path, current, '">', current,
              '</a></div>'].join(""));
          } else {
            el.addClass('file');
            el.text(current);
          }
          c.append(el);
        });
      }

      var scope = this;

      tree.on('click', '.treeExplorerExpander', function(e){
        if (!scope.settings.autoScroll){
          e.preventDefault();
        }

        var node = $(this).parent().parent();

        if (node.hasClass('open')){ //Are we open already?
          node.removeClass('open');
          return;
        } else { //We need to open
          if (node.find('.treeExplorerNode').length > 0){ //We already have children
            node.addClass('open');
          } else { //We need to get the children.
            var path = node.attr('id');
            lookup(path, function(path, children){
              if (children.length === 0){
                node.addClass('file');
                var name = node.find('a').text().replace(/\/$/, "");
                node.empty().text(name);
              } else {
                append(path, children, node);
                node.addClass('open');
              }
            });
          }
        }

      });

      getChildren("/", function(children){
        append("", children, tree);
      });

      return this;

    }
  })
})();
