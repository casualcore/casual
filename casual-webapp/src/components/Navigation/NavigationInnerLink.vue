<template>


    <router-link :to="{name: getName}" class="navi-anchor">
        <div class="navi-item navi-item__inner" :class="active">
                    <span class="navi-item__inner__text" :class="className">
                        {{name}}
                        <!--div class="navi__particle navi__particle-text navi__particle-text__groups">{{name}}</div-->
                    </span>
        </div>
        <slot></slot>
    </router-link>
</template>

<script>
    export default {
        name: "NavigationInnerLink",
        props: ['name', 'active'],
        computed: {
            getName: function () {
                return this.name.toLowerCase();
            },
            className: function () {
                const name = 'navi-item__text--' + this.name.toLowerCase();
                return {
                    [name]: true
                }
            }
        },
    }
</script>

<style scoped>

</style>