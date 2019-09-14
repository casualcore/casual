<template>


    <div id="side">
        <div class="side__content">

            <div class="casual-logo">
                <router-link :to="{name: 'home'}">
                    <span class="casual-logo-img"></span>
                </router-link>
            </div>
            <div class="navi">

                <navigation-link :name="'Groups'" :active="isActive('groups')"/>

                <navigation-link :name="'Servers'" :active="isActive('servers')"/>

                <navigation-link :name="'Executables'" :active="isActive('executables')"/>

                <navigation-link :name="'Services'" :active="isActive('services')"/>


                <navigation-link :name="'Gateways'" :active="isActive('gateways')"/>

                <navigation-link :name="'Transactions'" :active="isActive('transactions')"/>

            </div>
        </div>
    </div>
</template>

<script>
    import NavigationLink from "./NavigationLink";

    export default {
        name: "Sidebar",
        components: {NavigationLink},
        data() {
            return {
                active: false
            }
        },
        methods: {
            isActive: function (link) {
                let currentRoute = this.$route.fullPath.split('/console/').splice(1).join();
                let active = false;
                try {
                    active = currentRoute.startsWith(link);
                } catch (er) {
                    active = false;
                }
                return {
                    selected: active
                }
            }
        }
    }
</script>

<style scoped>

</style>