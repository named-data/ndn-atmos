/** NDN-Atmos: Cataloging Service for distributed data originally developed
 *  for atmospheric science data
 *  Copyright (C) 2015 Colorado State University
 *
 *  NDN-Atmos is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NDN-Atmos is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NDN-Atmos.  If not, see <http://www.gnu.org/licenses/>.
**/

"use strict";
var ndn = require("ndn-js");
var key = require("./key");
var fs = require("fs");

function RetrieveData(){
    this.pipelineSize = 4;
    this.timeoutDict = {};
    this.retransmissionDict = {};
    this.existingFileList = [];
    //FIXME: get this from the config file
    this.face = new ndn.Face({host: "localhost", port: "6363"});

    this.identityStorage = new ndn.MemoryIdentityStorage();
    this.privateKeyStorage = new ndn.MemoryPrivateKeyStorage();

    this.keyChain = new ndn.KeyChain(new ndn.IdentityManager(this.identityStorage,
                                    this.privateKeyStorage),
                                    new ndn.SelfVerifyPolicyManager(this.identityStorage));

    //FIXME:this should be based on the key
    this.keyName = new ndn.Name("/retrieve/DSK-123");
    this.certificateName = this.keyName.getSubName(0, this.keyName.size() - 1)
                          .append("KEY").append(this.keyName.get(-1))
                          .append("ID-CERT").append("0");

    this.identityStorage.addKey(this.keyName, ndn.KeyType.RSA,
                               new ndn.Blob(key.DEFAULT_RSA_PUBLIC_KEY_DER, false));

    this.privateKeyStorage.setKeyPairForKeyName(this.keyName,
                                               ndn.KeyType.RSA,
                                               key.DEFAULT_RSA_PUBLIC_KEY_DER,
                                               key.DEFAULT_RSA_PRIVATE_KEY_DER);

    this.face.setCommandSigningInfo(this.keyChain, this.certificateName);

    this.retrievePrefix = new ndn.Name("/retrieve/nwsc");
    this.face.registerPrefix(this.retrievePrefix, this.onUiInterest.bind(this),
                            this.onRegisterFailed.bind(this));
    console.log("Registering prefix " + this.retrievePrefix.toUri());
}


RetrieveData.prototype.onUiInterest = function(prefix, interest, face, interestFilterId, filter) {
    //FIXME: Authenticate Interest
    var interestName = new ndn.Name(interest.getName());
    var outgoingInterestName = interestName.getSubName(2).appendSegment(0);
    console.log("Received Interest" + interest.getName().toUri() +
               " Requesting " + outgoingInterestName.getName());
    //send this Interest is from the UI, bind it to a different onData call
    face.expressInterest(outgoingInterestName, this.onUiData.bind(this), this.onTimeout.bind(this));
    //using HTTP status codes
    var retrieveStatus = "200";
    var statusData = new ndn.Data(interest.getName());
    statusData.setContent(retrieveStatus);
    this.keyChain.sign(statusData, this.certificateName);
    this.face.putData(statusData);

}

RetrieveData.prototype.onUiData = function(Interest, data){
//this only handles UI data
    var payload = JSON.parse(data.getContent());
    console.log("Data is " + payload);

    //go over the loop and ask for each data object in the list
    for (var entry in payload) {
        //FIXME: if files exist locally, don't pull them.
        //FIXME: What if data is more than one packet? Make the UI create packets upto a certain size
        console.log(payload[entry]); //"aa", bb", "cc"
        var outgoingInterestName = new ndn.Name(payload[entry]);
        this.face.expressInterest(outgoingInterestName, this.onData.bind(this),
                                 this.onTimeout.bind(this));


        }
    }


RetrieveData.prototype.onData = function(interest, data) {
//this handles normal data
    var payload = data.getContent();
    var dataName = new ndn.Name(data.getName());
    var segment = dataName.get(-1).toSegment();
    var truncatedName = dataName.getPrefix(-1);
    var dataFileName = truncatedName.toUri().replace(/\//g, "_");
    //how do you data is from the UI? Last component is UI
    var lastComponent = new ndn.Name(truncatedName.getSubName(-1));

    //keep a hashmap of data names and check if data is out of order
    //if so, drop and rerequest

    //fetch the actual data
    if (segment === 0){
        this.retransmissionDict[dataName] = 0;
        console.log("Logged data segment" + dataName);
        fs.writeFile(dataFileName, payload, function(err){
            if(err){
                return console.log(err);
            }
        });
    }
    else{
        fs.appendFile(dataFileName, payload, function(err){
            if(err){
                return console.log(err);
            }
        });
    }

    if (data.getMetaInfo().getFinalBlockID() == data.getName()[-1]){
        fs.close(dataFileName);
        console.log("Closed file" + dataFileName);
    }
    else{
        //if out of order segment, rerequest
        if (segment - this.retransmissionDict[dataName] > 1){
            var reexpressInterest = truncatedName.appendSegment(this.retransmissionDict[dataName] + 1);
            this.face.expressInterest(reexpressInterest, this.onData.bind(this), this.onTimeout.bind(this));
        }
        else{
            this.retransmissionDict[dataName] = segment;
            var nextName = truncatedName.appendSegment(segment + this.pipelineSize);
            this.face.expressInterest(nextName, this.onData.bind(this), this.onTimeout.bind(this));
        }
    }

}

RetrieveData.prototype.onTimeout = function(interest) {
    var dupInterestName = interest.getName().toUri();
    console.log("Interest timeout for : " + dupInterestName);

    if(!Boolean(this.timeoutDict.hasOwnProperty(dupInterestName))){
        this.timeoutDict[dupInterestName] = 1;
        console.log("Logged timeout Interest" + dupInterestName);
    }
    if(this.timeoutDict[dupInterestName]!== 3){
        this.timeoutDict[dupInterestName] = this.timeoutDict[dupInterestName]++;
        console.log("Retry " + dupInterestName);
        this.face.expressInterest(interest, this.onData.bind(this), this.onTimeout.bind(this));
    }
    else{
        console.log("No data received for " + dupInterestName + ", giving up");
    }
}

RetrieveData.prototype.onRegisterFailed = function(prefix) {
    console.log("Registration failed for URI: " + prefix.toUri() + " Closing face");
    this.face.close();
 }


var main = function(){
    var run = new RetrieveData();
}

if (require.main === module) {
    main();
}
