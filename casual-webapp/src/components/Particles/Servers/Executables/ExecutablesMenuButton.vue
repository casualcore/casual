<template>
    <div class="navi__particle dropout-nav">
        <router-link :to="{name: 'executables'}">
            <a class="">
                <div class="navi__particle">
                    <span class="navi__particle-text navi__particle-text__executables">Exectuables</span>

                </div>
            </a>
        </router-link>
    </div>

</template>

<script>
    export default {
        name: "ExecutablesMenuButton"
    }
</script>

<style scoped>

</style>