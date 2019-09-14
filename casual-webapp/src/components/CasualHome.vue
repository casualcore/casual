<template>
    <div>

        <h3>Casual Home</h3>

        <hr>
        <h4>0.5.0 (2019-09-10)</h4>
        <ul>
            <li>Backend express-server completely abandoned. Everything is now static js/html</li>
            <li>Executables tab same size as the others once again</li>
        </ul>
        <hr>
        <h4>0.3.10 (2019-06-16)</h4>
        <ul>
            <li>Removes third party css-styles</li>
            <li>Removes sidebar on hover navigation button</li>
        </ul>
        <hr>
        <h4>0.3.11 (2019-06-29)</h4>
        <ul>
            <li>Services can be filtered by category</li>
            <li>Simple sort of services</li>
        </ul>
        <hr>
        <h4>0.3.10 (2019-06-16)</h4>
        <ul>
            <li>Removes third party css-styles</li>
            <li>Removes sidebar on hover navigation button</li>
        </ul>
        <hr>
        <h4>0.3.9 (2019-06-08)</h4>
        <ul>
            <li>Service Describer only visible if available</li>
        </ul>
        <hr>
        <h4>0.3.8 (2019-06-08)</h4>
        <ul>
            <li>Service view updated with API on compatible services</li>
            <li>New colors in Service time charts</li>

        </ul>
        <hr>
        <h4>0.3.7 (2019-06-06)</h4>
        <ul>
            <li>Visualization added to service call and pending times</li>
            <li>More info is shown in Services table</li>

        </ul>
        <hr>
        <h4>0.3.6 (2019-06-02)</h4>
        <ul>
            <li>Service now show some metrics
            </li>
        </ul>
        <hr>
        <h4>0.3.5 (2019-05-27)</h4>
        <ul>
            <li>Basic Gateways and Transactions views implemented
            </li>
        </ul>
        <hr>
        <h4>0.3.4 (2019-05-23)</h4>
        <ul>
            <li>Servers can now be scaled
                <ul>
                    <li>To a minimum of 1 instance</li>
                </ul>
            </li>
        </ul>

        <hr>
        <h4>0.3.3 (2019-05-22)</h4>
        <ul>
            <li>Executable view implemented</li>
            <li>A group now shows members as Servers or Executables</li>
            <li>Server show Group membership</li>
        </ul>

        <hr>
        <h4>0.3.2 (2019-05-21)</h4>
        <ul>
            <li>Basic Service view implemented</li>
        </ul>
        <hr>
        <h4>0.3.1 (2019-05-19)</h4>
        <ul>
            <li>Web GUI gets an update</li>
            <li>Particle view of Group and Server are now visible</li>
        </ul>
        <hr>
        <h4>0.3.0 (2019-05-15)</h4>
        <ul>
            <li><i><b>All features from previous builds are not yet implemented</b></i></li>
            <li>Web application is now built with Nodejs and Vue</li>
            <li>First simple list view of Groups, Servers and Services implemented</li>
            <li>Pages for particle information prepared</li>
        </ul>

    </div>
</template>

<script>
    export default {
        name: 'CasualHome',
        props: {
            msg: String
        }
    }
</script>

