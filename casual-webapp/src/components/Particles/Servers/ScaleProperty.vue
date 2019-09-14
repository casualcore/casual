<template>
    <div class="particle-property">
        <div class="particle-property__property-content__item"> <span class="scale-instance">Scale</span></div>
        <div class="particle-property__property-content__item">
            <ScaleButton :value="1" :text="'^'" @scale="scaleServer"/>
            <ScaleButton :value="-1" :text="'v'" @scale="scaleServer"/>
        </div>
        <div class="particle-property__name">
         <span class="scale-instance">  {{instances}}</span>
        </div>

    </div>
</template>

<script>
    import ScaleButton from "./ScaleButton";

    export default {
        name: "ScaleProperty",
        components: {ScaleButton},
        props: ['instances'],
        methods: {
            scaleServer: function (ins) {
                this.$emit('scale', ins);
            }
        }
    }
</script>

<style scoped>

</style>