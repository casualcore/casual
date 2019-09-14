<template>


    <div class="particle-property">
        <div class="particle-property--name">{{name}}</div>
        <div class="property-content__item">
            {{value}}
        </div>
    </div>


</template>

<script>
    export default {
        name: "SimpleProperty",
        props: ['name', 'value'],
    }
</script>

<style scoped>

</style>