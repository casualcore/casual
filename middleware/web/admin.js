var app = angular.module('myCasualAdminApp', []);

google.load('visualization', '1', {packages: ['corechart']});

function CasualAdminCtrl($scope, $http, $log, $timeout) {
	
	var promise;
	var host="http://localhost"
	
	var timeout=1000;
		
	$http.defaults.cache = false;
	
	$scope.serverInstancesMap = {};
	$scope.serverMap = {};
	$scope.servers = [];
	$scope.services = [];
	$scope.instances = [];
	$scope.selectedInstances = [];
	$scope.buttonlabel = "edit";
	$scope.editing = false;
	$scope.buttonclass = "btn btn-sm btn-success";
	
	function Instance(alias, instances)
	{
		this.alias = alias;
		this.instances = instances;
	}
	
	$scope.submit = function() {
		doGetCasualStateInfo();
		promise = $timeout( $scope.submit, timeout, false )
	}
		
	$scope.update = function() {
		$scope.commit();
		$log.log("update");
	}

	$scope.cancelTimeout = function() {
		$timeout.cancel(promise)
	}
	
	$scope.commit = function() {
		//
		// do some updating
		//
		$log.log("updating...");
		var myInstanceArr = new Array();
		for (var i = 0; i < $scope.servers.length; i++) {
  			var element = $scope.servers[i];
  			var servername = element.alias;
  			if (element.instances.length != element.instances.viewlength)
  			{
  				$log.info("updating: servername=" + servername + ", instances=" + element.instances.viewlength);
  				var aInstance = new Instance( servername, element.instances.viewlength)
  				myInstanceArr.push( aInstance)
  				uppdateInstances( myInstanceArr)
  			}
  			element.editing = false
 		}
		$scope.submit();
	}
	
	$scope.showInstance = function( serverInstances)
	{
	   $scope.selectedInstances = [];
	   for (var i=0; i < serverInstances.length; i++)
	   {
            for (var j = 0; j < $scope.instances.length; j++)
            {
               var instance = $scope.instances[j]
               if ( serverInstances[i] == instance.process.pid )
               {
                  $scope.selectedInstances.push(instance)
                  break
               }
            }
	   }
	   $scope.selectedInstances = instances;
	}
	
	$scope.getServer = function( pid)
	{
		if (pid in $scope.serverMap)
			return $scope.serverMap[pid]
		else
			return "admin"
	}

    $scope.getState = function( state)
    {
		//
		// TODO: Create map
		//
        if (state == 2)
			 return "IDLE"
        else
          return "BUSY"
    }
	
    function uppdateInstances( instanceArr)
    {
    	var jsonstring = angular.toJson( instanceArr)
    	$scope.instances = [];
    	$scope.selectedInstances = [];
    	$log.log(jsonstring)
		$http.defaults.headers.common.Accept = 'application/json';
		$http
				.post(host + '/casual/casual?service=.casual.broker.update.instances&protocol=json', '{"instances":' + jsonstring + '}')
				.success(uppdateInstancesCallback)
				.error(errorCallback);   	
    }

	function uppdateInstancesCallback(reply) {
		
		$log.log(reply);
	}
    
	function doGetCasualStateInfo() {
		$http.defaults.headers.common.Accept = 'application/json';
		$http
				.post(host + '/casual/casual?service=.casual.broker.state&protocol=json','{}')
				.success(getCasualStateInfoCallback)
				.error(errorCallback);
	}

	$scope.getMonitorStatistics = function doGetCasualMonitorInfo() {
		$http.defaults.headers.common.Accept = 'application/json';
		$http
				.post(host + '/casual/casual?service=getMonitorStatistics&protocol=json','{}')
				.success(getCasualMonitorInfoCallback)
				.error(errorCallback);
	}
	function getCasualMonitorInfoCallback(reply) 
	{	
	  $scope.outputValues = reply.outputValues;
	  drawBackgroundColor()
	  //$log.log($scope.outputValues);
	}
	
	function getCasualStateInfoCallback(reply) 
	{	
	  $scope.state = reply.serviceReturn;
      $scope.servers = $scope.state.servers;
      $scope.services = $scope.state.services;
      $scope.instances = $scope.state.instances;

      for (var i = 0; i < $scope.servers.length; i++) 
      {
         var element = $scope.servers[i];
         var servername = element.alias;
         for (var j = 0; j < element.instances.length; j++)
         {
            $scope.serverMap[element.instances[j]] = servername;
            $scope.instances[j].alias = servername;
         }
         
         if (! element.editing)
         {
        	 $scope.serverInstancesMap[servername] = element.instances.length;
        	 element.instances.viewlength = element.instances.length;
         }
         else
         {
        	 element.instances.viewlength = $scope.serverInstancesMap[servername]
         }
      
      }
      
      for (var i = 0; i < $scope.instances.length; i++) 
      {
         $scope.instances[i].alias = $scope.getServer( $scope.instances[i].process.pid)
      }
      
      if ($scope.selectedInstances.length == 0)
      {
         $scope.selectedInstances = $scope.instances;
      }
      else
      {
         for (var i = 0; i < $scope.selectedInstances.length; i++)
         {
            var selectedInstance = $scope.selectedInstances[i]
            for (var j = 0; j < $scope.instances.length; j++)
            {
               var instance = $scope.instances[j]
               if ( selectedInstance.process.pid == instance.process.pid )
               {
                  $scope.selectedInstances[i] = $scope.instances[j]
                  break
               }
            }
 
         }
      
      }

	}

	function drawBackgroundColor() {
		  
	      var data = new google.visualization.DataTable();
	      data.addColumn('datetime', 'X');
	      data.addColumn('number', 'getMonitorSt...');

	      for (i=0; i < $scope.outputValues.length; i++)
	      {
	    	  if ($scope.outputValues[i].service == "getMonitorStatistics")
	    	  {
		    	  start = $scope.outputValues[i].start/1000
		    	  end = $scope.outputValues[i].end/1000
		    	  data.addRow([new Date(start/1000), end - start])
	    	  }
	      }
	      
	      var options = {
	        hAxis: {
	          title: 'Timepoint'
	        },
	        vAxis: {
	          title: 'Elapsed time (usec)'
	        },
	        backgroundColor: '#f1f8e9',
	        width: 700,
	        height: 400
	      };

	      var chart = new google.visualization.ScatterChart(document.getElementById('chart_div'));
	      chart.draw(data, options);
	}
	
	function errorCallback(reply) {
		
		$log.error(reply);
	}

	//
	// start cycle
	//
	$scope.getMonitorStatistics()
	$scope.submit();

}

app.directive('ngEnter', function () {
    return function (scope, element, attrs) {
        element.bind("keydown keypress", function (event) {
            if(event.which === 13) {
                scope.$apply(function (){
                    scope.$eval(attrs.ngEnter);
                });

                event.preventDefault();
            }
        });
    };
});



