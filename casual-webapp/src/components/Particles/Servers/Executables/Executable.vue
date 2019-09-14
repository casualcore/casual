<template>
    <div>

        <h3>{{$options.name}}</h3>


        <ExecutableInfo v-if="executable !== ''" :executable="executable"></ExecutableInfo>
    </div>
</template>

<script>
    import ExecutableInfo from "./ExecutableInfo";

    export default {
        name: "Executable",
        components: {ExecutableInfo},
        data() {
            return {
                executable: '',
                executableAlias: this.$route.query.alias
            }
        },
        methods: {
            updateExecutbleInfo: async function () {


                this.executable = await this.Casual.getExecutableByAlias(this.executableAlias);


            }
        },
        created() {
            this.updateExecutbleInfo();
        }
    }
</script>

<style scoped>

</style>