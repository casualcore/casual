var app = angular.module('myCasualAdminApp', []);

function CasualAdminCtrl($scope, $http) {
	
	$http.defaults.cache = false;
	
	$scope.serverMap = {};
	$scope.servers = [];
	$scope.services = [];
	$scope.instances = [];

	doGetCasualServerInfo();
	doGetCasualServiceInfo();
	
	$scope.submit = function() {
		console.log("submit");
		doGetCasualServerInfo();
		doGetCasualServiceInfo();
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
		console.log($scope.servers);
		
		for (var i = 0; i < $scope.servers.length; i++) {
  			var element = $scope.servers[i];
  			var servername = element.alias;
  			for (var j = 0; j < element.instances.length; j++)
  			{
  				$scope.serverMap[element.instances[j].pid] = servername;
  			}	
 		}
 		console.log( $scope.serverMap)
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
		console.log($scope.services);
	}



	function errorCallback(reply) {
		
		console.log(reply);
	}

	
}
