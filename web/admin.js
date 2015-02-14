var app = angular.module('myCasualAdminApp', []);

function CasualAdminCtrl($scope, $http, $log, $timeout) {
	
	var promise;
	
	$http.defaults.cache = false;
	
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
		promise = $timeout( $scope.submit, 500, false )
	}
	
	$scope.commit = function() {
		if ($scope.buttonlabel == "edit")
		{
			$scope.buttonlabel = "commit";
			$scope.buttonclass = "btn btn-sm btn-danger";
			$scope.editing = true;
			//
			// cancel timeout to be able to edit
			//
			$timeout.cancel( promise);
		}
		else
		{
			$scope.buttonlabel = "edit";
			$scope.buttonclass = "btn btn-sm btn-success";
			$scope.editing = false;
			//
			// do some updating
			//
			var myInstanceArr = new Array();
			for (var i = 0; i < $scope.servers.length; i++) {
	  			var element = $scope.servers[i];
	  			var servername = element.alias;
	  			if (element.instances.length != element.instances.newlength)
	  			{
	  				$log.info("updating: servername=" + servername + ", instances=" + element.instances.newlength);
	  				var aInstance = new Instance( servername, element.instances.newlength)
	  				myInstanceArr.push( aInstance)
	  				uppdateInstances( myInstanceArr)
	  			}
	 		}
			
			//
			// start timeout again
			//
			promise = $timeout( $scope.submit, 500, false );
		}
	}
	
	$scope.showInstance = function( serverInstances)
	{
	   $scope.selectedInstances = [];
	   for (var i=0; i < serverInstances.length; i++)
	   {
            for (var j = 0; j < $scope.instances.length; j++)
            {
               var instance = $scope.instances[j]
               if ( serverInstances[i] == instance.pid )
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
				.post('http://localhost/casual/casual?service=.casual.broker.update.instances&protocol=json', '{"instances":' + jsonstring + '}')
				.success(uppdateInstancesCallback)
				.error(errorCallback);   	
    }

	function uppdateInstancesCallback(reply) {
		
		$log.log(reply);
	}
    
	function doGetCasualStateInfo() {
		$http.defaults.headers.common.Accept = 'application/json';
		$http
				.post('http://localhost/casual/casual?service=.casual.broker.state&protocol=json','{}')
				.success(getCasualStateInfoCallback)
				.error(errorCallback);
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
         element.instances.newlength = element.instances.length
         
      }
      
      for (var i = 0; i < $scope.instances.length; i++) 
      {
         $scope.instances[i].alias = $scope.getServer( $scope.instances[i].pid)
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
               if ( selectedInstance.pid == instance.pid )
               {
                  $scope.selectedInstances[i] = $scope.instances[j]
                  break
               }
            }
 
         }
      
      }

	}


	function errorCallback(reply) {
		
		$log.error(reply);
	}


	//
	// start cycle
	//
	$scope.submit();

}

