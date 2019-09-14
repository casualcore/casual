<template>
    <div>
        <hr>

        <div class="particle-row">
            <div class="particle-col particle-col__md">

                <div class="particle-property">
                    <div class="particle-property--name">Dependencies</div>
                    <ul v-if="hasDependencies" class="particle-property__list-items">
                        <li v-for="dep in group.dependencyNames" :key="dep">

                            <router-link :to="{name: 'group', query: {name: dep}}">
                                <a>{{dep}}</a>
                            </router-link>
                        </li>
                    </ul>
                    <ul v-else class="particle-property__list-items">
                        <li>None</li>
                    </ul>
                </div>
            </div>
        </div>
        <hr>
        <div class="particle-row">
            <div class="particle-col particle-col__md">
                <div class="particle-property">
                    <div class="particle-property--name">Members</div>
                    <div v-if="group.members.servers.length > 0"
                         class="particle-property__property-content__item">
                        <div>Servers</div>
                        <ul class="particle-property__list-items">
                            <li class="particle-property__list-item"
                                v-for="(member, index) in group.members.servers"
                                :key="index">
                                <router-link :to="{name: 'server', query: {alias: member}}">
                                    <a>{{member}}</a>
                                </router-link>

                            </li>
                        </ul>

                        <div v-if="group.members.executables.length > 0"
                             class="particle-property__property-content__item">

                            <div>Executables</div>
                            <ul class="particle-property__list-items">
                                <li class="particle-property__list-item"
                                    v-for="(member, index) in group.members.executables"
                                    :key="index">
                                    <router-link :to="{name: 'executable', query: {alias: member}}">
                                        <a>{{member}}</a>
                                    </router-link>

                                </li>
                            </ul>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </div>
</template>

<script>
    export default {
        name: "GroupInfo",
        props: ['group', 'hasDependencies']
    }
</script>

<style scoped>

</style>