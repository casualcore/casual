<template>
    <div>
        <DividerLine/>
        <div class="particle-row">

            <div class="particle-col particle-col__md">
                <SimpleProperty :name="'Alias'" :value="executable.alias"/>
            </div>
        </div>
        <DividerLine/>

        <div class="particle-row">

            <div class="particle-col particle-col__md">
                <SimpleProperty :name="'Path'" :value="executable.path"/>
            </div>
        </div>
        <div class="particle-row">

            <div class="particle-col particle-col__md">
                <SimpleProperty :name="'Arguments'" :value="executable.arguments.join(' ')"/>
            </div>
        </div>

        <DividerLine/>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <div class="particle-property">
                    <div class="particle-property--name">Member of</div>
                    <ul class="particle-property__list-items">
                        <li v-for="(member, index) in executable.membershipNames" :key="index">
                            <router-link :to="{name: 'group', query: {name: member}}">{{member}}</router-link>
                        </li>

                    </ul>
                </div>
            </div>
        </div>

    </div>
</template>

<script>
    import DividerLine from "../../../utilities/DividerLine";
    import SimpleProperty from "../../../Properties/SimpleProperty";

    export default {
        name: "ExecutableInfo",
        components: {SimpleProperty, DividerLine},
        props: ['executable']
    }
</script>

<style scoped>

</style>