<template>
    <div>

        <h3>{{$options.name}}</h3>


        <ServerInfo v-if="server !== ''" @scale="scale" :server="server" :scalable="canScale"></ServerInfo>


    </div>


</template>

<script>


    import ServerInfo from "./ServerInfo";

    export default {
        name: "Server",
        components: {ServerInfo},
        data() {
            return {
                serverAlias: this.$route.query.alias,
                currentServer: '',
                instances: 0,
                scalable: true,

            }
        },
        created: async function() {
            await this.updateServer();
        },
        computed: {
            server: function () {
                return this.currentServer;
            },
            canScale: function(){

                return !this.currentServer.alias.startsWith('.casual');
            }
        },
        methods: {
            updateServer: async function () {
                this.toggle();
                this.currentServer = await this.Casual.getServerByName(this.serverAlias);
            },
            scale: async function (ins) {

                this.toggle();
                await this.Casual.scaleInstance(this.server.alias, ins);
                this.updateServer();

            },
            toggle: function () {
                this.active = !this.active;

            }
        }
    }
</script>

<style scoped>

</style>