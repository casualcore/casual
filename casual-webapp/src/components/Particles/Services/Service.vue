<template>
    <div>
        <h3>{{$options.name}}</h3>

        <ServiceInfo v-if="service" :service="service"></ServiceInfo>
    </div>

</template>

<script>
    import ServiceInfo from "./ServiceInfo";

    export default {
        name: "Services",
        components: {ServiceInfo},
        data() {
            return {
                serviceName: this.$route.query.name,
                serviceBody: '',
            }
        },
        methods: {
            updateServiceInfo: async function () {

                this.serviceBody = await this.Casual.getServiceByName(this.serviceName);

            }
        },
        created() {
            this.updateServiceInfo();
        },
        computed: {
            service: function () {
                return this.serviceBody;
            }
        }

    }
</script>

<style scoped>

</style>