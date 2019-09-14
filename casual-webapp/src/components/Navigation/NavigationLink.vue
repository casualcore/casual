<template>
    <div>

        <router-link :to="{name: getName}" class="navi-anchor">
            <div class="navi-item " :class="active">
                    <span class="navi-item__text" :class="className">
                        {{name}}
                    </span>
            </div>

        </router-link>

        <slot></slot>

    </div>

</template>

<script>
    export default {
        name: "NavigationLink",
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
        }

    }
</script>

<style scoped>

</style>