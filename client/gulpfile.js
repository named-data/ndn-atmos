//Includes
var gulp = require('gulp');
var minifyHTML = require('gulp-minify-html');
var minifyCSS = require('gulp-minify-css');
var sourcemaps = require('gulp-sourcemaps');
var closure = require('gulp-closure-compiler-service');
var clean = require('gulp-clean');

//Globs
var cssGlob  = ['./catalog-dev/css/style.css', './catalog-dev/css/cubeLoader.css'];
var jsGlob   = './catalog-dev/js/*.js';
var htmlGlob = './catalog-dev/index.html';

gulp.task('minify-html', function() {

  return gulp.src(htmlGlob)
    .pipe(minifyHTML())
    .pipe(gulp.dest('./catalog/'));

});

gulp.task('minify-js', function() {

  return gulp.src(jsGlob, {base: 'catalog-dev'})
    .pipe(sourcemaps.init())
    .pipe(closure())
    .pipe(sourcemaps.write({sourceRoot: '/catalog-dev', includeContent: false}))
    .pipe(gulp.dest('./catalog'));

});

gulp.task('minify-css', function() {

  return gulp.src(cssGlob, {base: 'catalog-dev'})
    .pipe(sourcemaps.init())
    .pipe(minifyCSS())
    .pipe(sourcemaps.write({sourceRoot: '/catalog-dev', includeContent: false}))
    .pipe(gulp.dest('./catalog'));

});

gulp.task('copy', function() {

  gulp.src('./catalog-dev/config.json')
    .pipe(gulp.dest('./catalog'));

  gulp.src('./catalog-dev/css/*.min.css')
    .pipe(gulp.dest('./catalog/css'));

});

gulp.task('clean', function(){

  return gulp.src('./catalog', {read: false})
    .pipe(clean());

});

gulp.task('watch', ['default'], function(){

  gulp.watch(cssGlob, ['minify-css']);
  gulp.watch(jsGlob, ['minify-js']);
  gulp.watch(htmlGlob, ['minify-html']);
  gulp.watch(['./catalog-dev/config.json', './catalog-dev/css/*.min.css'], ['copy']);

});

gulp.task('default', ['minify-html', 'minify-css', 'minify-js', 'copy']);

