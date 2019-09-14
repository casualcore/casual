<template>
    <div>
        <h3>{{$options.name}}</h3>


        <AllExecutablesTable v-if="executables !== ''" :executables="executables"></AllExecutablesTable>
        <UpdateButton @refreshTable="refreshTable" :value="refreshButtonText"></UpdateButton>

    </div>
</template>

<script>
    import UpdateButton from "../../../utilities/SimpleButton";
    import AllExecutablesTable from "./AllExecutablesTable";

    export default {
        name: "Executables",
        components: {UpdateButton, AllExecutablesTable},
        data() {
            return {
                executables: ''
            }
        },
        methods: {
            refreshTable: async function () {

                this.executables = await this.Casual.getAllExecutables();
            }
        },
        created() {
            this.refreshTable();
        }
    }
</script>

<style scoped>

</style>