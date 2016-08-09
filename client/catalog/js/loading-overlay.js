/**
 * This function allows the app to pass in a callback function for updating the progress and a function to call if the progress it cancelled.
 * @param func - A function accepting two functions as parameters, the first will be called with one parameter (which is a callback function
 * for progress updates) when the overlay is ready.
 * @param cancel - A function to call if the overlay is cancelled.
 *
 * Example:
 * openLoadingOverlay(function(update){
 *   //Begin progress, call update if progress is in chunks, otherwise ignore.
 * }, function(){
 *   //Cancelled
 * });
 */
var openLoadingOverlay = (function(){
  "use strict";

  var isOpen = false;

  return function(func, cancel){

    var overlay = $('#loading');
    var progress = overlay.find('.progress-bar');
    var cancelButton = overlay.find('#loading-cancel');

    if (isOpen){
      console.warn("Two overlays are not permitted at the same time. The second will have dummy callbacks");
      func(function(){});
      return;
    }

    isOpen = true;

    var update = function(done, current, total){
      if (current && total){
        progress.text(current + '/' + total)
        .animate({
            width: Math.round(current/total) + '%'
          },
          200,
          'linear',
          function(){
            if (done){
              reset();
            }
          }
        );
      } else {
        if (done){
          reset();
        }
      }
    };

    var reset = function(){
      overlay.modal('hide');
      progress.text('Loading...').css('width', '100%');
      overlay.removeClass('cancelled').addClass('loading');
      cancelButton.removeClass('disabled');
    };

    cancelButton.one('click', function(){

      cancelButton.addClass('disabled');
      overlay.addClass('cancelled').removeClass('loading');

      setTimeout(reset, 2000);
      progress.text('Cancelling...').css('width', 0).animate({
        width: '100%'
      }, 2000);

    });

    overlay.modal('show');

    func(update);

  };
})();
