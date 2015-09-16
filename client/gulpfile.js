var gulp = require('gulp');
var minifyHTML = require('gulp-minify-html');
var minifyCSS = require('gulp-minify-css');
var sourcemaps = require('gulp-sourcemaps');
var closure = require('gulp-closure-compiler-service');
var clean = require('gulp-clean');

gulp.task('minify-html', function() {

  return gulp.src(['./catalog-dev/index.html'])
    .pipe(minifyHTML())
    .pipe(gulp.dest('./catalog/'));

});

gulp.task('minify-js', function() {

  return gulp.src('./catalog-dev/js/*.js')
    .pipe(sourcemaps.init())
    .pipe(closure())
    .pipe(sourcemaps.write('../../catalog-dev/js'))
    .pipe(gulp.dest('./catalog/js'));

});

gulp.task('minify-css', function() {

  return gulp.src(['./catalog-dev/css/style.css', './catalog-dev/css/cubeLoader.css'])
    .pipe(sourcemaps.init())
    .pipe(minifyCSS())
    .pipe(sourcemaps.write('../../catalog-dev/css'))
    .pipe(gulp.dest('./catalog/css'));

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

gulp.task('default', ['minify-html', 'minify-css', 'minify-js', 'copy']);

