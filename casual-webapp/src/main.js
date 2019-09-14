import Vue from 'vue'
import App from './App.vue'
import router from './router';
import axios from 'axios';
import {mixin} from './mixins/mixins';
import store from './store/store';
import VueGoogleCharts from 'vue-google-charts';
import {orderBy} from 'lodash';


Vue.use(VueGoogleCharts);

const ax = axios.create({});

Object.defineProperty(Vue.prototype, '$orderBy', {value: orderBy});
Vue.prototype.$http = ax;


Vue.config.productionTip = false;

Vue.filter('nanoToSeconds', function (nano) {

    let billion = 1000000000;

    return (nano / billion).toFixed(6);

});

Vue.mixin(mixin);
new Vue({
    router,
    store,
    render: h => h(App),
}).$mount('#app')
