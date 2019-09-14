<template>
    <div>

        <h3>{{$options.name}}</h3>

        <AllGroupsTable v-if="groups !== ''" :groups="groups"></AllGroupsTable>


        <UpdateButton @go="refreshTable" :value="refreshButtonText"></UpdateButton>

    </div>
</template>

<script>
    import AllGroupsTable from "./AllGroupsTable";
    import UpdateButton from "../../utilities/SimpleButton";

    export default {
        name: "Groups",
        components: {UpdateButton, AllGroupsTable},
        data() {
            return {
                groups: ''
            }
        },
        created() {
            this.refreshTable();
        },
        methods: {
            refreshTable: async function () {

                this.groups = await this.Casual.getAllGroups();

            }
        }
    }
</script>

<style scoped>

</style>