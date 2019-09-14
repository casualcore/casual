<template>
    <div>

        <h3>{{$options.name}}</h3>


        <AllServersTable v-if="servers !== ''" :servers="servers"></AllServersTable>

        <SimpleButton @go="refreshTable" :value="refreshButtonText"></SimpleButton>
    </div>
</template>

<script>


    import AllServersTable from "./AllServersTable";
    import SimpleButton from "../../utilities/SimpleButton";

    export default {
        name: "Servers",
        components: {SimpleButton, AllServersTable},
        data() {
            return {
                servers: ''
            }
        },
        methods: {
            refreshTable: async function () {
                this.servers =  await this.Casual.getAllServers();
            }
        },
        created() {
            this.refreshTable();
        }

    }
</script>

<style scoped>

</style>