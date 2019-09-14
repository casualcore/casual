
let domain = require('./backend/domain.json');
let service = require('./backend/service.json');
let transactions = require('./backend/transaction.json');
let gateway = require('./backend/gateway.json');



function getData(json) {

   let splitJson = json.split('/');
   let jsonData = '';
   switch(splitJson[3]) {
      case 'domain':
         jsonData = domain;
         break;
      case 'service':
         jsonData = service;
         break;
      case 'transaction':
         jsonData = transactions;
         break;
      case 'gateway':
         jsonData = gateway;
         break;
      default:
   }

   return JSON.stringify(jsonData);
}



module.exports = getData;