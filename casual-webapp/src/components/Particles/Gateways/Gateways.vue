<template>
    <div>


        <h3>{{$options.name}}</h3>

        <AllGatewaysTable :gateways="gateways"></AllGatewaysTable>


        <UpdateButton @go="updateGateways" :value="refreshButtonText"></UpdateButton>


    </div>

</template>

<script>
    import AllGatewaysTable from "./AllGatewaysTable";
    import UpdateButton from '../../utilities/SimpleButton';

    export default {
        name: "Gateways",
        components: {AllGatewaysTable, UpdateButton},
        data() {
            return {
                gateways: ''
            }
        },
        methods: {
            updateGateways: async function () {

                this.gateways = await this.Casual.getAllGateways();
            }
        },
        created() {
            this.updateGateways();
        }
    }
</script>

<style scoped>

</style>