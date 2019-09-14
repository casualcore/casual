<template>
    <div>

        <h3>{{$options.name}}</h3>


        <AllTransactionsTable :transactions="transactions"></AllTransactionsTable>
        <SimpleButton @go="updateTransactions" :value="refreshButtonText"></SimpleButton>
    </div>
</template>

<script>
    import AllTransactionsTable from './AllTransactionsTable';
    import SimpleButton from '../../utilities/SimpleButton';

    export default {
        name: "Transactions",
        components: {SimpleButton,AllTransactionsTable},
        data() {
            return {
                transactions: ''
            }
        },
        methods: {
            updateTransactions: async function () {

                this.transactions = await this.Casual.getAllTransactions();
            }
        },
        created() {
            this.updateTransactions();
        }
    }
</script>

<style scoped>

</style>