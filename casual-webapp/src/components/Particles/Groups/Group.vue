<template>
    <div>

        <h3>{{$options.name}} {{groupName}}</h3>



        <GroupInfo v-if="group.id" :group="group" :hasDependencies="hasDependencies"></GroupInfo>

    </div>
</template>

<script>
    import GroupInfo from "./GroupInfo";

    export default {
        name: "Group",
        components: {GroupInfo},
        data() {
            return {
                groupName: this.$route.query.name,
                groupBody: {
                    dependencyNames: []
                },

            }
        },
        methods: {
            updateGroupInfo: async function () {

                this.groupBody = await this.Casual.getGroupByName(this.groupName);

            }
        },
        created() {
            this.updateGroupInfo();
            this.$store.commit('setTitleName', this.groupName);
        },
        computed: {
            hasDependencies: function () {
                return this.groupBody.dependencyNames.length > 0;
            },
            group: function () {
                return this.groupBody;
            }
        },
        watch: {
            '$route.query.name': function(){
                this.groupName = this.$route.query.name;
                this.updateGroupInfo();
            }
        }
    }
</script>

<style scoped>

</style>