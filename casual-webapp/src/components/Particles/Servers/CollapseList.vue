<template>
    <div>

        <div class="particle-property">
            <div class="particle-property--name">Services</div>
            <div class="collapse">
                <div class="collapse-container">

                    <ul class="particle-property__list-items">
                        <li v-for="service in services" :key="service.id">
                            <router-link :to="{name: 'service', query: {name: service}}">
                                <a>{{service}}</a>
                            </router-link>
                        </li>
                    </ul>
                </div>
            </div>
        </div>


    </div>


</template>

<script>
    export default {
        name: "CollapseList",
        props: ['services'],

    }
</script>

<style scoped>

    .collapse-container {
        height: 100%;

    }



</style>