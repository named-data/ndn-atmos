"use strict";
const fs = require('fs');

const regex = /^Original File Name: ([\w\d-]+\.nc)\s*^NDN Name: (\/(?:[\w\d-]+\/)*)/gm;

fs.readFile('sample_output', 'utf8',function(err, data){

  if (err){
    return console.error(err);
  }

  var output = {};
  for (let match = regex.exec(data); match; match = regex.exec(data)){
    if (output[match[2]]){
      console.log("Duplicate found for:", match[2], "|", output[match[2]], "|", match[1]);
      continue;
    }
    output[match[2]] = match[1];
  }

  console.log("Found", Object.keys(output).length, "conversions.");

  fs.writeFile('conversions.json', JSON.stringify(output), function(err){
    if (err){
      return console.error(err);
    }	
  });

});

