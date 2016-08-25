Casual - web application
-------------
Webapplication built with the power of web components and based on the [Polymer Starter Kit](https://developers.google.com/web/tools/polymer-starter-kit/?hl=en).
For full installation instructions follow the Polymer App Toolbox instructions (link at end of page).

#### Setup for development:
> 1. Install Node.s, LTS version (4.x)
> 2. Install Polymer CLI:  

```npm install -g polymer-cli ```

> 3. Goto casual/webapp/
> 4. Start local server:

```polymer serve --open```

>5. Goto: http://localhost:8080/
>6. Done!

#### Bower
Bower is used to fetch and install web components. To install a new web component from the [Polymer catalog](https://elements.polymer-project.org/) type:

```bower install --save "element-name"```

All components uploaded to bower can be installed using this command.

#### Deployment:
See full instructions on: [Polymer Deploy](https://www.polymer-project.org/1.0/start/toolbox/deploy). 
Type the following command:

```Polymer build```
> This command minifies the HTML, JS, and CSS dependencies of your application, and generates a service worker that precaches all of the dependencies of your application so that it can work offline.
> The built files are output to the following folders:
> build/unbundled. Contains granular resources suitable for serving via HTTP/2 with server push.
> build/bundled. Contains bundled (concatenated) resources suitable for serving from servers or to clients that do not support HTTP/2 server push.

#### Current features: 
- Four views list each of the following data: Servers, Groups, Instances and Services.
- Deployable with Polymer CLI (Pre-release)

#### Todo:
- Relative URLs for REST-call.
- Call REST service for real time data.

#### Good to know:
- [Polymer App Toolbox](https://www.polymer-project.org/1.0/start/toolbox/set-up) - Starter toolbox for simplified website development and web component introduction.
- [Polymer Catalog](https://elements.polymer-project.org/) - Catalog over all Google Polymer elements.
- [Customelements.io](https://customelements.io/) - Website for web components created with polymer.