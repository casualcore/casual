<template>
    <div>
        <divider-line/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <div class="particle-property">
                    <div class="particle-property--name">Instances</div>
                    <scale-property v-if="scalable" class="test" @scale="scale" :instances="server.instances.length"/>
                </div>
            </div>

        </div>
        <divider-line/>


        <div class="particle-row">
            <div class="particle-col particle-col__md">

                <collapse-list :services="server.services"/>
            </div>
        </div>
        <divider-line/>

        <div class="particle-row">
            <div class="particle-col particle-col__md">

                <div class="particle-property">
                    <div class="particle-property--name">Member of</div>
                    <ul class="particle-property__list-items">
                        <li v-for="group in server.membershipNames" :key="group.name">
                            <router-link :to="{name: 'group', query: {name: group}}">
                                <a>{{group}}</a>
                            </router-link>
                        </li>
                    </ul>
                </div>
            </div>
        </div>
        <divider-line/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <simple-property :name="'Note'" :value="server.note"/>
            </div>
        </div>
        <divider-line/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <simple-property :name="'Path'" :value="server.path"/>
            </div>
        </div>

    </div>
</template>

<script>
    import ScaleProperty from "./ScaleProperty";
    import SimpleProperty from "../../Properties/SimpleProperty";
    import DividerLine from "../../utilities/DividerLine";
    import CollapseList from "./CollapseList";

    export default {
        name: "ServerInfo",
        components: {CollapseList, DividerLine, SimpleProperty, ScaleProperty},
        props: ['server', 'scalable'],
        computed: {
            instances: function () {
                return this.server.instances.length;
            }
        },
        methods: {
            scale: function (ins) {
                this.$emit('scale', ins);
            }

        }


    }
</script>

<style scoped>


    #Progress_Status {
        width: 50%;
        background-color: #ddd;
    }

    #myprogressBar {
        width: 10%;
        height: 35px;
        background-color: #4CAF50;
        text-align: center;
        line-height: 32px;
        color: black;
    }


</style>