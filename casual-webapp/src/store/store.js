import Vue from 'vue';
import Vuex from 'vuex';

Vue.use(Vuex);

export default new Vuex.Store({
    state: {
        user: {
            name: ''
        },
        nameTitle: ''
    },
    getters: {
        loggedInUser: function (state) {
            return state.user;
        },
        getTitle: function (state) {
            return state.nameTitle;
        }
    },
    mutations: {
        logIn: function (state, username) {
            state.user.name = username;
        },
        setTitleName: function (state, name) {

            state.nameTitle = name;
        }
    }
})
