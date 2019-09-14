<template>

    <div>
        <Topbar/>
        <Sidebar/>
        <div class="content" id="content">
            <router-view/>
        </div>
        <CasualFooter/>
    </div>


</template>


<script>
    import Sidebar from './Navigation/Sidebar';
    import Topbar from './Navigation/Topbar';
    import CasualFooter from "./CasualFooter";

    export default {
        name: "Casual",
        components: {CasualFooter, Sidebar, Topbar}
    }
</script>

<style scoped>

</style>