<template>

    <div class="particle-property">
        <div class="particle-property__property-name">Services</div>
        <ul class="particle-property__list-items">
            <li v-for="particle in particles" :key="particle.id">
                <router-link :to="{'name': particleName, 'query': { [attr] : particle}}">
                    <a>{{particle}}</a>
                </router-link>
            </li>
        </ul>
    </div>
</template>

<script>
    export default {
        name: "SimpleLinkProperty",
        props: ['particles', 'queryAttr', 'particleName'],
        data() {
            return {
                attr: this.queryAttr
            }
        },
        methods: {
        }
    }
</script>

<style scoped>

</style>