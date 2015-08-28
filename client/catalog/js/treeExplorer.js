
(function(){
  "use strict";
  jQuery.fn.extend({
    treeExplorer: function(getChildren){

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
            el.append(['<a href="#' , path , current , '">' , current , '</a>'].join(""));
          } else {
            el.addClass('file');
            el.text(current);
          }
          c.append(el);
        });
      }

      tree.on('click', '.treeExplorerNode > a', function(){
        var node = $(this).parent();

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
