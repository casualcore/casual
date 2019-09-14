<template>
    <div>
        <div class="particle-top">
            <div class="page-title">{{$options.name}}</div>
            <div class="filter-attributes">
                <div class="particle-row">
                    <div class="particle-col particle-col__md">
                        <div class="particle-property">
                            <div class="particle-property--name"> Filter by:</div>
                            <div class="particle-property--wrapper">
                                <div class="place--row">
                                    <filtering ref="filter" v-if="filterOptions('category').length > 0"
                                               :type="'category'"
                                               :filterOptions="filterOptions('category')"
                                               @filterBy="filterBy">
                                        Category:
                                    </filtering>
                                    <small-button @go="clearFilter" :value="'Reset'">Reset</small-button>
                                </div>
                            </div>
                        </div>
                    </div>
                    <div class="particle-col particle-col__md">
                        <div class="particle-property">
                            <div class="particle-property--name"> Sort by:</div>
                            <div class="particle-property--wrapper">
                                <div class="place--row">
                                    <sorting ref="sorting" v-if="filterOptions('category').length > 0"
                                             :sortingOptions="sortingOptions()"
                                             @sortBy="sortBy">
                                    </sorting>
                                    <small-button @go="reverseSort" :value="'Reverse'">Reverse</small-button>
                                    <small-button @go="clearFilter" :value="'Reset'">Reset</small-button>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

        </div>


        <AllServicesTable v-if="filtered.length > 0" :services="filtered"></AllServicesTable>


        <simple-button @go="refreshTable" :value="refreshButtonText"></simple-button>


    </div>

</template>

<script>
    import AllServicesTable from "./AllServicesTable";
    import SimpleButton from "../../utilities/SimpleButton";
    import SmallButton from "../../utilities/SmallButton";
    import Filtering from "../../utilities/Filtering";
    import Sorting from "../../utilities/Sorting";
    import {filtermixin} from "../../../mixins/filtermixin";


    export default {
        name: "Services",
        components: {Filtering, Sorting, SimpleButton, SmallButton, AllServicesTable},
        mixins: [filtermixin],
        data() {
            return {
                services: '',
                reverse: false,
                filtered: '',
                filters: {},
                fOptions: {
                    category: ''
                },
                sOptions: []
            }
        },
        created() {
            this.refreshTable();

        },

        computed: {
            reversed: function () {
                let rev = '';
                if (this.reverse) {
                    rev = 'desc';
                } else {
                    rev = 'asc';
                }
                return rev;


            }
        },
        methods: {
            refreshTable: async function () {
                this.services = await this.Casual.getAllServices();
                this.filtered = this.services;

                //this.$refs.sorting.resetSort();

            },
            filterOptions: function (type) {
                const options = this.getOptions(this.services, type);
                //console.log(options);
                this.fOptions[type] = options;
                return options;
            },
            filterBy: function (option, type) {
                this.filters[type] = option;
                if (option === '' && this.filters.hasOwnProperty(type)) {
                    delete this.filters[type];
                }
                this.filtered = this.filterObject(this.services, this.filters);
            },
            sortingOptions: function () {

                return [
                    {
                        name: 'Name',
                        value: 'name'
                    },
                    {

                        name: 'Category',
                        value: 'category'
                    },
                    {
                        name: 'Calls',
                        value: 'metrics.count'
                    },
                    {
                        name: 'Average',
                        value: 'metrics.average'
                    },
                    {
                        name: 'Min',
                        value: 'metrics.limit.min'
                    },
                    {
                        name: 'Max',
                        value: 'metrics.limit.max'
                    },
                    {
                        name: 'Pending',
                        value: 'pending.count'
                    },
                    {
                        name: 'Pending Average',
                        value: 'pending.average'
                    },
                    {
                        name: 'Last',
                        value: 'last'
                    }
                ];
            },
            reverseSort: function () {

                this.filtered.reverse();


            },
            sortBy: function (option) {

                this.reverse = false;
                this.filtered = this.$orderBy(this.filtered, option, 'asc');

            },
            clearFilter: function () {

                this.filtered = this.services;
                this.$refs.filter.reset();
                this.$refs.sorting.reset();
                this.fOptions = {};
            }

        },
        watch: {}

    }
</script>

<style scoped>

</style>