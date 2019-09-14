<template>
    <div>


        <table class="table-style">
            <thead>
            <tr>
                <th scope="col">Name</th>
                <th scope="col">Category</th>
                <th scope="col">Calls</th>
                <th scope="col">Average</th>
                <th scope="col">Min</th>
                <th scope="col">Max</th>
                <th scope="col">Pending</th>
                <th scope="col">Pending avg</th>
                <th scope="col">Last</th>
            </tr>
            </thead>
            <tbody>

            <tr v-for="service in services" :key="service.name">
                <td>
                    <router-link :to="{name: 'service', query: {name: service.name}}">
                        <a>{{service.name || '-'}}</a>
                    </router-link>
                </td>
                <td>{{service.category || '-'}}</td>
                <td>{{service.metrics.count}}</td>
                <td>{{service.metrics.average | nanoToSeconds}}</td>
                <td>{{service.metrics.limit.min | nanoToSeconds}}</td>
                <td>{{service.metrics.limit.max | nanoToSeconds}}</td>
                <td>{{service.pending.count}}</td>
                <td>{{service.pending.average | nanoToSeconds}}</td>
                <td>{{formatDate(service.last)}}</td>

            </tr>

            </tbody>
        </table>


    </div>
</template>

<script>
    export default {
        name: "AllServicesTable",
        props: ['services'],
        data() {
            return {
                allServices: '',
                sortBy: '',
                order: ''
            }
        },
        created() {
            this.allServices = this.services;
        },
        computed:
            {
                sorted: function () {
                    return this.$orderBy(this.services, this.sortBy, this.order);
                }
            },
        methods: {
            lastCalled: function (nano) {
                return this.formatDate(nano);
            },
            sort: function (type, order) {

                this.sortBy = type;
                this.order = order;


            }
        }
    }
</script>

<style scoped>

</style>