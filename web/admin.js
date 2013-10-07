var app = angular.module('myCasualAdminApp', []);

function CasualAdminCtrl($scope, $http, $log, $timeout) {
	
	var promise;
	
	$http.defaults.cache = false;
	
	$scope.serverMap = {};
	$scope.servers = [];
	$scope.services = [];
	$scope.instances = [];
	$scope.buttonlabel = "edit";
	$scope.editing = false;
	$scope.buttonclass = "btn btn-sm btn-success"
	
	$scope.submit = function() {
		$log.info("submit");
		doGetCasualServerInfo();
		doGetCasualServiceInfo();
		promise = $timeout( $scope.submit, 5000, false )
	}
	
	$scope.commit = function() {
		$log.info("commit");
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
			for (var i = 0; i < $scope.servers.length; i++) {
	  			var element = $scope.servers[i];
	  			var servername = element.alias;
	  			if (element.instances.length != element.instances.newlength)
	  			{
	  				$log.info("updating: servername=" + servername + ", instances=" + element.instances.newlength);
	  			}
	 		}
			
			//
			// start timeout again
			//
			promise = $timeout( $scope.submit, 5000, false );
		}
	}
	
	$scope.showInstance = function( instances)
	{
		$scope.instances = instances;
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
            return "unknown"
    }
		
	function doGetCasualServerInfo() {
		$http.defaults.headers.common.Accept = 'application/json';
		$http
				.post('http://casual.laz.se/test1/casual/casual?service=_broker_listServers&protocol=JSON','{}')
				.success(getCasualServerInfoCallback)
				.error(errorCallback);
	}

	function getCasualServerInfoCallback(reply) {
		
		$scope.servers = reply.serviceReturn;
		$log.log($scope.servers);
		
		for (var i = 0; i < $scope.servers.length; i++) {
  			var element = $scope.servers[i];
  			var servername = element.alias;
  			for (var j = 0; j < element.instances.length; j++)
  			{
  				$scope.serverMap[element.instances[j].pid] = servername;
  			}
  			element.instances.newlength = element.instances.length
 		}
		$log.log( $scope.serverMap)
	}

	function doGetCasualServiceInfo() {
		$http.defaults.headers.common.Accept = 'application/json';
		$http
				.post('http://casual.laz.se/test1/casual/casual?service=_broker_listServices&protocol=JSON','{}')
				.success(getCasualServiceInfoCallback)
				.error(errorCallback);
	}

	function getCasualServiceInfoCallback(reply) {
		
		$scope.services = reply.serviceReturn;
		$log.log($scope.services);
	}



	function errorCallback(reply) {
		
		$log.error(reply);
	}


	//
	// start cycle
	//
	$scope.submit();

}

