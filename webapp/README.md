Casual - web application
-------------
Webapplication built with the power of web components and based on the [Polymer Starter Kit](https://developers.google.com/web/tools/polymer-starter-kit/?hl=en).

#### Setup for development:
> 1. Install [bower](http://bower.io/).
> 2. Go to: /casual/webapp
> 2.  Type: bower install 
> 3.  Done!

#### Bower
Bower is used to fetch and install web components. To install a new web component from the [Polymer catalog](https://elements.polymer-project.org/) type:
> bower install - -save "element-name"

All components uploaded to bower can be installed using this command.

#### Current features: 
- Simple table listing of one REST-service example.
- Current REST-data is static from json-file.
- Utilizes [paper-datatable](https://customelements.io/David-Mulder/paper-datatable/).
- Four views list each of the following data: Servers, Groups, Instances and Services.
- The webapplication can be previewed on the following website: [alexandra-eriksson.se](http://alexandra-eriksson.se/#!/servers)

#### Todo:
- Relative URLs for REST-call.
- Call REST service for real time data.
- Create better views over REST-data. 
	- Remove the use of paper-datatable.
	- Combine current views to present relevant data from different perspectives.
- Use [Gulp](https://github.com/gulpjs/gulp) or similar to build the application and to do other things such as minification of files.  

#### Good to know:
- [Polymer Starter Kit](https://developers.google.com/web/tools/polymer-starter-kit/?hl=en) - Starter kit for simplified website development and web component introduction.
- [Polymer Catalog](https://elements.polymer-project.org/) - Catalog over all Google Polymer elements.
- [Customelements.io](https://customelements.io/) - Website for web components created with polymer.