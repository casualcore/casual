<template>
    <div>
        <DividerLine/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">

                <SimpleProperty :name="'Service'"
                                :value="service.name"></SimpleProperty>
            </div>
        </div>
        <DividerLine/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <div class="particle-property">
                    <div class="particle-property--name">Server</div>
                    <div class="particle-property__property-content__item">
                        <router-link :to="{name: 'server', query: {alias: service.parentServer.alias}}">
                            {{service.parentServer.alias}}
                        </router-link>
                    </div>
                </div>
            </div>
        </div>
        <DividerLine/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <SimpleProperty :name="'Category'" :value="service.category"></SimpleProperty>
            </div>
        </div>
        <DividerLine/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <SimpleProperty :name="'Last Called'" :value="formatDate(service.last)"></SimpleProperty>
            </div>
        </div>
        <DividerLine/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <div class="particle-property">
                    <div class="particle-property--name">Calls</div>
                    <div class="particle-property--wrapper">
                        <Metrics :metric="metrics"/>
                    </div>
                </div>
            </div>
            <MetricChart :metric="metrics" :title="'Time spent in service'"/>
        </div>
        <DividerLine/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <div class="particle-property">
                    <div class="particle-property--name">Pending</div>
                    <div class="particle-property--wrapper">
                        <Metrics :metric="pending"/>

                    </div>
                </div>
            </div>
            <MetricChart :metric="pending" :title="'Time spent wating for service'"/>
        </div>
        <DividerLine/>
        <div class="particle-row" v-if="hasApi">
            <div class="particle-col particle-col__xs">
                <div class="particle-property--name">Service Describer
                    <SimpleButton @go="toggleApi" :value="show ? 'Hide API' :'Show API'"></SimpleButton>
                </div>
            </div>
            <div class="particle-col" v-if="show">
                <div class="">Service</div>
                <div class="api-describer api-describer__highlight">{{serviceApi.service}}</div>
                <DividerLine/>
                <div class="">Input</div>

                <api-describe v-if="serviceApi.input" :api="serviceApi.input"></api-describe>
                <DividerLine/>
                <div class="">Output</div>

                <api-describe v-if="serviceApi.output" :api="serviceApi.output"></api-describe>
            </div>

        </div>
    </div>
</template>

<script>

    import SimpleProperty from "../../Properties/SimpleProperty";
    import DividerLine from "../../utilities/DividerLine";
    import Metrics from "./Metrics";
    import MetricChart from "./MetricChart";
    import apiDescribe from "./api-describe";
    import SimpleButton from "../../utilities/SimpleButton";

    export default {
        name: "ServiceInfo",
        components: {apiDescribe, MetricChart, Metrics, DividerLine, SimpleProperty, SimpleButton},
        props: ['service'],
        data() {
            return {
                metrics: this.service.metrics,
                pending: this.service.pending,
                serviceApi: {
                    service: 'none'
                },
                show: false
            }
        },
        created: async function () {
            await this.checkApi();
        },
        methods: {
            checkApi: async function () {

                this.serviceApi = await this.Casual.getServiceApi(this.service.name);
            },
            getYaml: function (json) {
                return this.toYaml(json);
            },
            toggleApi: function () {
                this.show = !this.show;
            }


        },
        computed: {
            showapi: function () {
                return this.show;
            },
            hasApi: function () {
                return this.serviceApi.service !== 'none';


            }
        }


    }
</script>

<style scoped>

</style>