<template>
    <div class="particle-col particle-col__sm">
        <GChart type="ColumnChart"
                :data="chartData"
                :options="options"/>
    </div>
</template>

<script>
    export default {
        name: "MetricChart",
        props: ["metric", "title"],
        data() {
            return {}
        },
        computed: {
            chartData: function () {
                const min = this.metric.limit.min;
                const max = this.metric.limit.max;
                const average = this.metric.average;
                return [
                    ["Time", "Min", "Max", "Average"],
                    ['Nanoseconds', min, max, average]
                ];


            },
            options: function () {



                return {
                    vAxis: {minValue: 0, format: '###,###,###'},
                    colors: ['#292e34', '#1d2124', '#3a424a'],
                    height: 150,
                    chartArea: {
                        width: "50%",
                        left: "20%",
                    },
                    legend: {},
                    bar: {
                        groupWidth: "65%"
                    },
                    title: this.title,
                    hAxis: {
                        baselineColor: "transparent",
                        gridLines: {
                            color: "transparent"
                        }
                    },
                    ticks: 0
                };

            }
        },
        methods: {
            getAverage: function () {
                if (this.metric.count === 0) {
                    return 0;
                }
                let called = this.metric.count;
                let total = this.metric.total;
                return total / called;
            }
        }
    }
</script>

<style scoped>

</style>