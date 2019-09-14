<template>
    <div>

        <div class="particle-property__property-content__row">
            <SimpleProperty :name="'Count:'"
                            :value="metric.count"></SimpleProperty>
        </div>
            <div class="particle-property__property-content__row">
            <SimpleProperty :name="'Min:'"
                            :value="metric.limit.min | nanoToSeconds"></SimpleProperty>

        </div>
        <div class="particle-property__property-content__row">

            <SimpleProperty :name="'Max:'"
                            :value="metric.limit.max | nanoToSeconds"></SimpleProperty>
        </div>
        <div class="particle-property__property-content__row">

            <SimpleProperty :name="'Average:'"
                            :value="metric.average | nanoToSeconds"></SimpleProperty>

        </div>
        <div class="particle-property__property-content__row">

            <SimpleProperty :name="'Total:'"
                            :value="metric.total | nanoToSeconds"></SimpleProperty>

        </div>
    </div>
</template>

<script>

    import SimpleProperty from '../../Properties/SimpleProperty';

    export default {
        name: "Metrics",
        props: ['metric'],
        components: {SimpleProperty},
    }
</script>

<style scoped>

</style>